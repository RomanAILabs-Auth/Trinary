/*
 * tri_lang.c — Trinary v0 language front-end.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * Lexer + recursive-descent parser + direct kernel dispatch.
 * Accepts a strict subset of the .tri grammar (see language/spec/grammar.ebnf):
 *
 *   program      := (stmt NEWLINE)*
 *   stmt         := comment | print_stmt | decl | par_block | braincore_stmt
 *   comment      := '#' any* '\n'
 *   print_stmt   := 'print' '(' STRING ')'
 *   decl         := IDENT ':' type '=' expr                          # trit[N] = V
 *                 | IDENT ':' type                                   # cl4[N]   (metadata)
 *   type         := 'trit' '[' INT ']'
 *                 | 'cl4'  '[' INT ']'
 *   par_block    := 'par' 'spacetime' ':' NEWLINE INDENT loop DEDENT
 *                 | 'par' NEWLINE INDENT stmt+ DEDENT
 *   loop         := 'for' IDENT 'in' 'range' '(' INT ',' INT ')' ':' NEWLINE INDENT body DEDENT
 *   body         := IDENT '=' IDENT '*' '-' INT                      # lattice = lattice * -1
 *                 | IDENT '=' IDENT '.' 'rotate' '(' ')'             # rotor stub (ignored in v0)
 *   braincore_stmt := 'braincore'  ('(' INT (',' INT)? ')')?
 *
 * v0 dispatch rules:
 *   - `lattice: trit[N] = 1` allocates a bit-packed lattice (N trits -> N/8 bytes).
 *   - `par spacetime: for _ in range(0, ITER): lattice = lattice * -1`
 *       -> trinary_v1_lattice_flip(lattice, N/64, ITER, 0xFFFF...FF)
 *   - `braincore(N, I)` or bare `braincore`
 *       -> trinary_v1_braincore_u8 with N neurons, I iterations (defaults: 4M, 1000).
 *   - Anything else is a parse error with line/column diagnostic.
 */
#include "trinary/trinary.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Lexer                                                                     */
/* ------------------------------------------------------------------------- */

typedef enum {
    T_EOF, T_NEWLINE, T_INDENT, T_DEDENT,
    T_IDENT, T_INT, T_STRING,
    T_COLON, T_LPAREN, T_RPAREN, T_LBRACK, T_RBRACK,
    T_COMMA, T_EQUALS, T_STAR, T_MINUS, T_DOT,
    T_KW_PRINT, T_KW_PAR, T_KW_FOR, T_KW_IN, T_KW_RANGE,
    T_KW_TRIT, T_KW_CL4, T_KW_SPACETIME, T_KW_BRAINCORE, T_KW_SOA
} tok_kind;

typedef struct {
    tok_kind kind;
    const char *start;
    size_t len;
    long long ival;
    int line, col;
    int indent; /* for INDENT/DEDENT */
} tok_t;

typedef struct {
    const char *src;
    size_t len;
    size_t pos;
    int line, col;
    /* indent stack for python-like blocks */
    int indent_stack[64];
    int indent_depth;
    int pending_dedents;
    int at_line_start;
    tok_t lookahead;
    int has_lookahead;
} lexer_t;

static int lex_peek(lexer_t *L, size_t off) {
    return (L->pos + off < L->len) ? (unsigned char)L->src[L->pos + off] : -1;
}

static int lex_get(lexer_t *L) {
    if (L->pos >= L->len) return -1;
    int c = (unsigned char)L->src[L->pos++];
    if (c == '\n') { L->line++; L->col = 1; } else { L->col++; }
    return c;
}

static void lex_init(lexer_t *L, const char *src, size_t len) {
    memset(L, 0, sizeof(*L));
    L->src = src; L->len = len; L->line = 1; L->col = 1;
    L->at_line_start = 1;
    L->indent_stack[0] = 0;
    L->indent_depth = 1;
}

static int kw_match(const char *s, size_t n, const char *kw) {
    size_t klen = strlen(kw);
    if (n != klen) return 0;
    return memcmp(s, kw, klen) == 0;
}

static tok_kind classify_ident(const char *s, size_t n) {
    if (kw_match(s, n, "print"))     return T_KW_PRINT;
    if (kw_match(s, n, "par"))       return T_KW_PAR;
    if (kw_match(s, n, "for"))       return T_KW_FOR;
    if (kw_match(s, n, "in"))        return T_KW_IN;
    if (kw_match(s, n, "range"))     return T_KW_RANGE;
    if (kw_match(s, n, "trit"))      return T_KW_TRIT;
    if (kw_match(s, n, "cl4"))       return T_KW_CL4;
    if (kw_match(s, n, "spacetime")) return T_KW_SPACETIME;
    if (kw_match(s, n, "braincore")) return T_KW_BRAINCORE;
    if (kw_match(s, n, "soa"))       return T_KW_SOA;
    return T_IDENT;
}

static int lex_next(lexer_t *L, tok_t *t) {
    memset(t, 0, sizeof(*t));

    if (L->pending_dedents > 0) {
        L->pending_dedents--;
        t->kind = T_DEDENT;
        t->line = L->line; t->col = L->col;
        return 0;
    }

    /* Handle indentation at the start of a logical line. */
    if (L->at_line_start) {
        int spaces = 0;
        while (L->pos < L->len) {
            int c = (unsigned char)L->src[L->pos];
            if (c == ' ')      { spaces++; L->pos++; L->col++; }
            else if (c == '\t'){ spaces += 4; L->pos++; L->col++; }
            else break;
        }
        /* Blank line or comment-only: ignore indentation. */
        int c0 = (L->pos < L->len) ? (unsigned char)L->src[L->pos] : -1;
        if (c0 == '\n' || c0 == '#' || c0 == -1) {
            L->at_line_start = 0;
            /* fall through to regular tokenization */
        } else {
            L->at_line_start = 0;
            int top = L->indent_stack[L->indent_depth - 1];
            if (spaces > top) {
                if (L->indent_depth >= 64) return -1;
                L->indent_stack[L->indent_depth++] = spaces;
                t->kind = T_INDENT;
                t->line = L->line; t->col = L->col;
                return 0;
            } else if (spaces < top) {
                while (L->indent_depth > 1 &&
                       L->indent_stack[L->indent_depth - 1] > spaces) {
                    L->indent_depth--;
                    L->pending_dedents++;
                }
                if (L->pending_dedents > 0) {
                    L->pending_dedents--;
                    t->kind = T_DEDENT;
                    t->line = L->line; t->col = L->col;
                    return 0;
                }
            }
        }
    }

    /* Skip inline whitespace. */
    while (L->pos < L->len) {
        int c = (unsigned char)L->src[L->pos];
        if (c == ' ' || c == '\t' || c == '\r') { L->pos++; L->col++; continue; }
        break;
    }

    if (L->pos >= L->len) {
        /* EOF: emit DEDENTs for each open indent, then EOF. */
        if (L->indent_depth > 1) {
            L->indent_depth--;
            t->kind = T_DEDENT;
            t->line = L->line; t->col = L->col;
            return 0;
        }
        t->kind = T_EOF;
        t->line = L->line; t->col = L->col;
        return 0;
    }

    int c = (unsigned char)L->src[L->pos];
    t->line = L->line; t->col = L->col;
    t->start = L->src + L->pos;

    /* Comment: consume to end of line, don't emit. */
    if (c == '#') {
        while (L->pos < L->len && L->src[L->pos] != '\n') { L->pos++; L->col++; }
        return lex_next(L, t);
    }

    if (c == '\n') {
        lex_get(L);
        L->at_line_start = 1;
        t->kind = T_NEWLINE;
        return 0;
    }

    if (isalpha(c) || c == '_') {
        size_t start = L->pos;
        while (L->pos < L->len) {
            int cc = (unsigned char)L->src[L->pos];
            if (!(isalnum(cc) || cc == '_')) break;
            L->pos++; L->col++;
        }
        t->start = L->src + start;
        t->len = L->pos - start;
        t->kind = classify_ident(t->start, t->len);
        return 0;
    }

    if (isdigit(c)) {
        long long v = 0;
        while (L->pos < L->len && isdigit((unsigned char)L->src[L->pos])) {
            v = v * 10 + (L->src[L->pos] - '0');
            L->pos++; L->col++;
        }
        t->ival = v;
        t->kind = T_INT;
        return 0;
    }

    if (c == '"') {
        L->pos++; L->col++;
        size_t start = L->pos;
        while (L->pos < L->len && L->src[L->pos] != '"' && L->src[L->pos] != '\n') {
            L->pos++; L->col++;
        }
        t->start = L->src + start;
        t->len = L->pos - start;
        if (L->pos < L->len && L->src[L->pos] == '"') { L->pos++; L->col++; }
        t->kind = T_STRING;
        return 0;
    }

    L->pos++; L->col++;
    switch (c) {
        case ':': t->kind = T_COLON;  return 0;
        case '(': t->kind = T_LPAREN; return 0;
        case ')': t->kind = T_RPAREN; return 0;
        case '[': t->kind = T_LBRACK; return 0;
        case ']': t->kind = T_RBRACK; return 0;
        case ',': t->kind = T_COMMA;  return 0;
        case '=': t->kind = T_EQUALS; return 0;
        case '*': t->kind = T_STAR;   return 0;
        case '-': t->kind = T_MINUS;  return 0;
        case '.': t->kind = T_DOT;    return 0;
        default:
            fprintf(stderr, "[tri] lex error: unexpected character '%c' at %d:%d\n",
                    c, t->line, t->col);
            return -1;
    }
}

static int peek(lexer_t *L, tok_t *out) {
    if (L->has_lookahead) { *out = L->lookahead; return 0; }
    int r = lex_next(L, &L->lookahead);
    if (r != 0) return r;
    L->has_lookahead = 1;
    *out = L->lookahead;
    return 0;
}

static int advance(lexer_t *L, tok_t *out) {
    if (L->has_lookahead) { *out = L->lookahead; L->has_lookahead = 0; return 0; }
    return lex_next(L, out);
}

/* ------------------------------------------------------------------------- */
/* Interpreter state                                                         */
/* ------------------------------------------------------------------------- */

typedef struct {
    char name[64];
    enum { VAR_NONE, VAR_LATTICE_TRIT, VAR_CL4 } kind;
    uint64_t *lattice;
    size_t lattice_trits;
    size_t lattice_words;
} tri_var_t;

#define MAX_VARS 16
typedef struct {
    tri_var_t vars[MAX_VARS];
    int nvars;
} tri_env_t;

static tri_var_t *env_get(tri_env_t *E, const char *name, size_t nlen) {
    for (int i = 0; i < E->nvars; ++i) {
        if (strlen(E->vars[i].name) == nlen && memcmp(E->vars[i].name, name, nlen) == 0)
            return &E->vars[i];
    }
    return NULL;
}

static tri_var_t *env_new(tri_env_t *E, const char *name, size_t nlen) {
    if (E->nvars >= MAX_VARS) return NULL;
    if (nlen >= sizeof(E->vars[0].name)) nlen = sizeof(E->vars[0].name) - 1;
    tri_var_t *v = &E->vars[E->nvars++];
    memset(v, 0, sizeof(*v));
    memcpy(v->name, name, nlen);
    v->name[nlen] = 0;
    return v;
}

static void env_free(tri_env_t *E) {
    for (int i = 0; i < E->nvars; ++i) {
        if (E->vars[i].lattice) trinary_v1_free(E->vars[i].lattice);
    }
}

/* ------------------------------------------------------------------------- */
/* Parser / interpreter                                                      */
/* ------------------------------------------------------------------------- */

#define EXPECT(L_, tok_, expected_, msg_) do { \
    if (advance((L_), &(tok_)) != 0) return -1; \
    if ((tok_).kind != (expected_)) { \
        fprintf(stderr, "[tri] parse error at %d:%d: expected %s\n", \
                (tok_).line, (tok_).col, (msg_)); \
        return -1; \
    } \
} while (0)

static int parse_decl(lexer_t *L, tri_env_t *E, tok_t ident) {
    tok_t t;
    EXPECT(L, t, T_COLON, "':' after identifier");

    /* Optional 'soa' prefix — accepted and ignored. */
    if (peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_SOA) { advance(L, &t); }

    if (advance(L, &t) != 0) return -1;
    if (t.kind == T_KW_TRIT) {
        EXPECT(L, t, T_LBRACK, "'['");
        if (advance(L, &t) != 0) return -1;
        long long n;
        if (t.kind == T_INT) {
            n = t.ival;
        } else {
            fprintf(stderr, "[tri] parse error at %d:%d: expected INT size\n",
                    t.line, t.col);
            return -1;
        }
        /* Optional '*' INT for multi-dim: multiply sizes. */
        while (1) {
            if (peek(L, &t) != 0) return -1;
            if (t.kind != T_STAR) break;
            advance(L, &t);
            EXPECT(L, t, T_INT, "INT size");
            n *= t.ival;
        }
        EXPECT(L, t, T_RBRACK, "']'");
        /* Optional '=' INT initializer. */
        long long init_val = 1;
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_EQUALS) {
            advance(L, &t);
            if (peek(L, &t) != 0) return -1;
            int neg = 0;
            if (t.kind == T_MINUS) { neg = 1; advance(L, &t); }
            EXPECT(L, t, T_INT, "INT initializer");
            init_val = neg ? -t.ival : t.ival;
        }
        /* Allocate bit-packed lattice: round count up to multiple of 256 bits. */
        size_t trits = (size_t)n;
        size_t words = (trits + 63) / 64;
        words = (words + 3) / 4 * 4;  /* align to 256-bit block */
        uint64_t pat = (init_val < 0) ? 0xFFFFFFFFFFFFFFFFULL : 0ULL;
        if (trits == 0) {
            fprintf(stderr, "[tri] error: zero-size lattice\n");
            return -1;
        }
        tri_var_t *v = env_new(E, ident.start, ident.len);
        if (!v) return -1;
        v->kind = VAR_LATTICE_TRIT;
        v->lattice = (uint64_t *)trinary_v1_alloc(words * sizeof(uint64_t), 32);
        if (!v->lattice) { fprintf(stderr, "[tri] alloc failed\n"); return -1; }
        for (size_t i = 0; i < words; ++i) v->lattice[i] = pat;
        v->lattice_trits = trits;
        v->lattice_words = words;
        return 0;
    }
    if (t.kind == T_KW_CL4) {
        EXPECT(L, t, T_LBRACK, "'['");
        EXPECT(L, t, T_INT,    "INT size");
        EXPECT(L, t, T_RBRACK, "']'");
        tri_var_t *v = env_new(E, ident.start, ident.len);
        if (!v) return -1;
        v->kind = VAR_CL4;
        return 0;
    }
    fprintf(stderr, "[tri] parse error at %d:%d: expected type 'trit' or 'cl4'\n",
            t.line, t.col);
    return -1;
}

/* Parse:  IDENT '=' IDENT ('*' '-' INT | '.' 'rotate' '(' ')' ) */
static int parse_assignment(lexer_t *L, tri_env_t *E,
                            tok_t lhs, int iterations_in_loop)
{
    tok_t t;
    EXPECT(L, t, T_EQUALS, "'='");
    tok_t rhs;
    EXPECT(L, rhs, T_IDENT, "identifier");

    /* lattice = lattice * -1  -> flip */
    if (peek(L, &t) != 0) return -1;
    if (t.kind == T_STAR) {
        advance(L, &t);
        int neg = 0;
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_MINUS) { neg = 1; advance(L, &t); }
        EXPECT(L, t, T_INT, "INT");
        long long mul = neg ? -t.ival : t.ival;
        if (mul != -1) {
            fprintf(stderr, "[tri] only '* -1' supported in v0 at %d:%d\n",
                    t.line, t.col);
            return -1;
        }
        /* Dispatch flip over lhs variable, iterations = iterations_in_loop. */
        tri_var_t *v = env_get(E, lhs.start, lhs.len);
        if (!v || v->kind != VAR_LATTICE_TRIT) {
            fprintf(stderr, "[tri] undefined lattice variable '%.*s'\n",
                    (int)lhs.len, lhs.start);
            return -1;
        }
        trinary_v1_status rc = trinary_v1_lattice_flip(
            v->lattice, v->lattice_words,
            (size_t)(iterations_in_loop > 0 ? iterations_in_loop : 1),
            0xFFFFFFFFFFFFFFFFULL);
        return rc == TRINARY_OK ? 0 : -1;
    }

    if (t.kind == T_DOT) {
        /* .rotate() — placeholder, succeeds silently in v0 */
        advance(L, &t);
        EXPECT(L, t, T_IDENT, "method name");
        EXPECT(L, t, T_LPAREN, "'('");
        EXPECT(L, t, T_RPAREN, "')'");
        return 0;
    }
    fprintf(stderr, "[tri] parse error at %d:%d: unsupported assignment\n",
            t.line, t.col);
    return -1;
}

/* Parse a for loop body — expects INDENT already consumed by caller. */
static int parse_loop_body(lexer_t *L, tri_env_t *E, int iterations) {
    tok_t t;
    while (1) {
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_DEDENT) { advance(L, &t); return 0; }
        if (t.kind == T_EOF)    return 0;
        if (t.kind == T_NEWLINE) { advance(L, &t); continue; }
        if (t.kind == T_IDENT) {
            tok_t lhs; advance(L, &lhs);
            if (parse_assignment(L, E, lhs, iterations) != 0) return -1;
            continue;
        }
        fprintf(stderr, "[tri] parse error at %d:%d: unexpected token in loop\n",
                t.line, t.col);
        return -1;
    }
}

/* Parse:  'for' IDENT 'in' 'range' '(' INT ',' INT ')' ':' NEWLINE INDENT body DEDENT */
static int parse_for(lexer_t *L, tri_env_t *E) {
    tok_t t;
    EXPECT(L, t, T_KW_FOR, "'for'");
    EXPECT(L, t, T_IDENT,  "iteration var");
    EXPECT(L, t, T_KW_IN,  "'in'");
    EXPECT(L, t, T_KW_RANGE, "'range'");
    EXPECT(L, t, T_LPAREN, "'('");
    EXPECT(L, t, T_INT,    "start");
    long long lo = t.ival;
    EXPECT(L, t, T_COMMA,  "','");
    EXPECT(L, t, T_INT,    "stop");
    long long hi = t.ival;
    EXPECT(L, t, T_RPAREN, "')'");
    EXPECT(L, t, T_COLON,  "':'");
    while (1) {
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_NEWLINE) { advance(L, &t); continue; }
        break;
    }
    EXPECT(L, t, T_INDENT, "indented block");
    int iters = (int)(hi > lo ? (hi - lo) : 0);
    return parse_loop_body(L, E, iters);
}

/* Parse 'par' ('spacetime'?) ':' NEWLINE INDENT ( for_loop | stmt+ ) DEDENT */
static int parse_par(lexer_t *L, tri_env_t *E) {
    tok_t t;
    EXPECT(L, t, T_KW_PAR, "'par'");
    /* Optional 'spacetime' or 'for' inline. */
    if (peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_SPACETIME) { advance(L, &t); }
    if (peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_FOR) {
        /* 'par for' inline style */
        return parse_for(L, E);
    }
    EXPECT(L, t, T_COLON, "':'");
    while (1) {
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_NEWLINE) { advance(L, &t); continue; }
        break;
    }
    EXPECT(L, t, T_INDENT, "indented block");
    /* Body may contain a single for-loop (common case) or stmts. */
    if (peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_FOR) {
        if (parse_for(L, E) != 0) return -1;
        /* consume closing DEDENT of par block */
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_DEDENT) advance(L, &t);
        return 0;
    }
    /* Generic stmt loop within par block. */
    while (1) {
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_DEDENT) { advance(L, &t); return 0; }
        if (t.kind == T_EOF)    return 0;
        if (t.kind == T_NEWLINE) { advance(L, &t); continue; }
        if (t.kind == T_IDENT) {
            tok_t lhs; advance(L, &lhs);
            if (parse_assignment(L, E, lhs, 1) != 0) return -1;
            continue;
        }
        fprintf(stderr, "[tri] parse error in par block at %d:%d\n", t.line, t.col);
        return -1;
    }
}

static int parse_print(lexer_t *L) {
    tok_t t;
    EXPECT(L, t, T_KW_PRINT, "'print'");
    EXPECT(L, t, T_LPAREN,   "'('");
    EXPECT(L, t, T_STRING,   "STRING");
    printf("%.*s\n", (int)t.len, t.start);
    fflush(stdout);
    EXPECT(L, t, T_RPAREN,   "')'");
    return 0;
}

static int parse_braincore(lexer_t *L) {
    tok_t t;
    EXPECT(L, t, T_KW_BRAINCORE, "'braincore'");
    long long neurons = 4000000;
    long long iter    = 1000;
    if (peek(L, &t) != 0) return -1;
    if (t.kind == T_LPAREN) {
        advance(L, &t);
        EXPECT(L, t, T_INT, "neurons");
        neurons = t.ival;
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_COMMA) {
            advance(L, &t);
            EXPECT(L, t, T_INT, "iterations");
            iter = t.ival;
        }
        EXPECT(L, t, T_RPAREN, "')'");
    }
    if (neurons <= 0 || neurons % 32 != 0) {
        neurons = (neurons + 31) / 32 * 32;
        if (neurons <= 0) neurons = 32;
    }
    uint8_t *pot = (uint8_t *)trinary_v1_alloc((size_t)neurons, 32);
    uint8_t *inp = (uint8_t *)trinary_v1_alloc((size_t)neurons, 32);
    if (!pot || !inp) { fprintf(stderr, "[tri] alloc failed\n"); return -1; }
    trinary_v1_rng_seed(0);
    trinary_v1_rng_fill(inp, (size_t)neurons);
    /* Scale inputs to avoid saturating too quickly. */
    for (long long i = 0; i < neurons; ++i) inp[i] %= 50;
    for (long long i = 0; i < neurons; ++i) pot[i] = 0;

    double t0 = trinary_v1_now_seconds();
    trinary_v1_status rc = trinary_v1_braincore_u8(pot, inp,
                                                   (size_t)neurons,
                                                   (size_t)iter, 200);
    double t1 = trinary_v1_now_seconds();
    trinary_v1_free(pot); trinary_v1_free(inp);
    if (rc != TRINARY_OK) return -1;
    double elapsed = t1 - t0;
    double ops = (double)neurons * (double)iter;
    printf("[trinary] braincore: %.4f s  (%.2f GNeurons/s, variant=%s)\n",
           elapsed, ops / elapsed / 1e9, trinary_v1_active_variant("braincore"));
    fflush(stdout);
    return 0;
}

static int parse_stmt(lexer_t *L, tri_env_t *E) {
    tok_t t;
    if (peek(L, &t) != 0) return -1;
    switch (t.kind) {
        case T_KW_PRINT:     return parse_print(L);
        case T_KW_PAR:       return parse_par(L, E);
        case T_KW_BRAINCORE: return parse_braincore(L);
        case T_IDENT: {
            tok_t ident; advance(L, &ident);
            /* lookahead: ':' -> decl,  '=' -> assignment */
            if (peek(L, &t) != 0) return -1;
            if (t.kind == T_COLON)  return parse_decl(L, E, ident);
            if (t.kind == T_EQUALS) return parse_assignment(L, E, ident, 1);
            fprintf(stderr, "[tri] parse error at %d:%d: bare identifier '%.*s'\n",
                    ident.line, ident.col, (int)ident.len, ident.start);
            return -1;
        }
        default:
            fprintf(stderr, "[tri] parse error at %d:%d: unexpected token\n",
                    t.line, t.col);
            return -1;
    }
}

static int parse_program(lexer_t *L, tri_env_t *E) {
    tok_t t;
    while (1) {
        if (peek(L, &t) != 0) return -1;
        if (t.kind == T_EOF) return 0;
        if (t.kind == T_NEWLINE || t.kind == T_DEDENT || t.kind == T_INDENT) {
            advance(L, &t); continue;
        }
        if (parse_stmt(L, E) != 0) return -1;
    }
}

/* ------------------------------------------------------------------------- */
/* Public entry points                                                       */
/* ------------------------------------------------------------------------- */

trinary_v1_status trinary_v1_run_source(const char *source, size_t len) {
    if (!source) return TRINARY_ERR_ARGS;
    if (len == 0) len = strlen(source);
    trinary_v1_init();
    lexer_t L;
    lex_init(&L, source, len);
    tri_env_t E = {0};
    int rc = parse_program(&L, &E);
    env_free(&E);
    return rc == 0 ? TRINARY_OK : TRINARY_ERR_PARSE;
}

trinary_v1_status trinary_v1_run_file(const char *path) {
    if (!path) return TRINARY_ERR_ARGS;
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[tri] cannot open file: %s\n", path);
        return TRINARY_ERR_IO;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return TRINARY_ERR_IO; }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return TRINARY_ERR_ALLOC; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = 0;
    trinary_v1_status rc = trinary_v1_run_source(buf, got);
    free(buf);
    return rc;
}
