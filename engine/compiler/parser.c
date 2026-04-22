/*
 * parser.c — Trinary v0 parser + interpreter (extracted from tri_lang.c).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"
#include "tri_frontend.h"
#include "sema.h"
#include "ir.h"
#include "optimizer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

#define EXPECT(L_, tok_, expected_, msg_) do { \
    if (tri_lex_advance((L_), &(tok_)) != 0) return -1; \
    if ((tok_).kind != (expected_)) { \
        fprintf(stderr, "[tri] parse error at %d:%d: expected %s\n", \
                (tok_).line, (tok_).col, (msg_)); \
        return -1; \
    } \
} while (0)

static int parse_assignment(lexer_t *L, tri_env_t *E, tok_t lhs, int iterations_in_loop);

static int parse_decl(lexer_t *L, tri_env_t *E, tok_t ident) {
    tok_t t;
    EXPECT(L, t, T_COLON, "':' after identifier");
    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_SOA) { tri_lex_advance(L, &t); }

    if (tri_lex_advance(L, &t) != 0) return -1;
    if (t.kind == T_KW_TRIT) {
        EXPECT(L, t, T_LBRACK, "'['");
        if (tri_lex_advance(L, &t) != 0) return -1;
        long long n;
        const int n_line = t.line;
        const int n_col = t.col;
        if (t.kind == T_INT) n = t.ival;
        else { fprintf(stderr, "[tri] parse error at %d:%d: expected INT size\n", t.line, t.col); return -1; }
        while (1) {
            if (tri_lex_peek(L, &t) != 0) return -1;
            if (t.kind != T_STAR) break;
            tri_lex_advance(L, &t);
            EXPECT(L, t, T_INT, "INT size");
            n *= t.ival;
        }
        EXPECT(L, t, T_RBRACK, "']'");
        long long init_val = 1;
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_EQUALS) {
            tri_lex_advance(L, &t);
            if (tri_lex_peek(L, &t) != 0) return -1;
            int neg = 0;
            if (t.kind == T_MINUS) { neg = 1; tri_lex_advance(L, &t); }
            EXPECT(L, t, T_INT, "INT initializer");
            init_val = neg ? -t.ival : t.ival;
        }
        if (tri_sema_validate_trit_decl(n, n_line, n_col) != 0) return -1;
        size_t trits = (size_t)n;
        size_t words = (trits + 63) / 64;
        words = (words + 3) / 4 * 4;
        uint64_t pat = (init_val < 0) ? 0xFFFFFFFFFFFFFFFFULL : 0ULL;
        if (trits == 0) { fprintf(stderr, "[tri] error: zero-size lattice\n"); return -1; }
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
        EXPECT(L, t, T_INT, "INT size");
        if (tri_sema_validate_cl4_decl(t.ival, t.line, t.col) != 0) return -1;
        EXPECT(L, t, T_RBRACK, "']'");
        tri_var_t *v = env_new(E, ident.start, ident.len);
        if (!v) return -1;
        v->kind = VAR_CL4;
        return 0;
    }
    fprintf(stderr, "[tri] parse error at %d:%d: expected type 'trit' or 'cl4'\n", t.line, t.col);
    return -1;
}

static int parse_assignment(lexer_t *L, tri_env_t *E, tok_t lhs, int iterations_in_loop) {
    tok_t t;
    EXPECT(L, t, T_EQUALS, "'='");
    tok_t rhs;
    EXPECT(L, rhs, T_IDENT, "identifier");

    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_STAR) {
        tri_lex_advance(L, &t);
        int neg = 0;
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_MINUS) { neg = 1; tri_lex_advance(L, &t); }
        EXPECT(L, t, T_INT, "INT");
        long long mul = neg ? -t.ival : t.ival;
        if (mul != -1) {
            fprintf(stderr, "[tri] only '* -1' supported in v0 at %d:%d\n", t.line, t.col);
            return -1;
        }
        tri_var_t *v = env_get(E, lhs.start, lhs.len);
        if (!v || v->kind != VAR_LATTICE_TRIT) {
            fprintf(stderr, "[tri] undefined lattice variable '%.*s'\n", (int)lhs.len, lhs.start);
            return -1;
        }
        trinary_v1_status rc = trinary_v1_lattice_flip(
            v->lattice, v->lattice_words,
            (size_t)(iterations_in_loop > 0 ? iterations_in_loop : 1),
            0xFFFFFFFFFFFFFFFFULL);
        return rc == TRINARY_OK ? 0 : -1;
    }

    if (t.kind == T_DOT) {
        tri_lex_advance(L, &t);
        EXPECT(L, t, T_IDENT, "method name");
        EXPECT(L, t, T_LPAREN, "'('");
        EXPECT(L, t, T_RPAREN, "')'");
        return 0;
    }
    fprintf(stderr, "[tri] parse error at %d:%d: unsupported assignment\n", t.line, t.col);
    return -1;
}

static int parse_loop_body(lexer_t *L, tri_env_t *E, int iterations) {
    tok_t t;
    while (1) {
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_DEDENT) { tri_lex_advance(L, &t); return 0; }
        if (t.kind == T_EOF) return 0;
        if (t.kind == T_NEWLINE) { tri_lex_advance(L, &t); continue; }
        if (t.kind == T_IDENT) {
            tok_t lhs; tri_lex_advance(L, &lhs);
            if (parse_assignment(L, E, lhs, iterations) != 0) return -1;
            continue;
        }
        fprintf(stderr, "[tri] parse error at %d:%d: unexpected token in loop\n", t.line, t.col);
        return -1;
    }
}

static int parse_for(lexer_t *L, tri_env_t *E) {
    tok_t t;
    EXPECT(L, t, T_KW_FOR, "'for'");
    EXPECT(L, t, T_IDENT, "iteration var");
    EXPECT(L, t, T_KW_IN, "'in'");
    EXPECT(L, t, T_KW_RANGE, "'range'");
    EXPECT(L, t, T_LPAREN, "'('");
    EXPECT(L, t, T_INT, "start");
    long long lo = t.ival;
    EXPECT(L, t, T_COMMA, "','");
    EXPECT(L, t, T_INT, "stop");
    long long hi = t.ival;
    if (tri_sema_validate_loop_range(lo, hi, t.line, t.col) != 0) return -1;
    EXPECT(L, t, T_RPAREN, "')'");
    EXPECT(L, t, T_COLON, "':'");
    while (1) {
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_NEWLINE) { tri_lex_advance(L, &t); continue; }
        break;
    }
    EXPECT(L, t, T_INDENT, "indented block");
    int iters = (int)(hi > lo ? (hi - lo) : 0);
    return parse_loop_body(L, E, iters);
}

static int parse_par(lexer_t *L, tri_env_t *E) {
    tok_t t;
    EXPECT(L, t, T_KW_PAR, "'par'");
    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_SPACETIME) { tri_lex_advance(L, &t); }
    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_FOR) return parse_for(L, E);
    EXPECT(L, t, T_COLON, "':'");
    while (1) {
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_NEWLINE) { tri_lex_advance(L, &t); continue; }
        break;
    }
    EXPECT(L, t, T_INDENT, "indented block");
    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_KW_FOR) {
        if (parse_for(L, E) != 0) return -1;
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_DEDENT) tri_lex_advance(L, &t);
        return 0;
    }
    while (1) {
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_DEDENT) { tri_lex_advance(L, &t); return 0; }
        if (t.kind == T_EOF) return 0;
        if (t.kind == T_NEWLINE) { tri_lex_advance(L, &t); continue; }
        if (t.kind == T_IDENT) {
            tok_t lhs; tri_lex_advance(L, &lhs);
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
    EXPECT(L, t, T_LPAREN, "'('");
    EXPECT(L, t, T_STRING, "STRING");
    printf("%.*s\n", (int)t.len, t.start);
    fflush(stdout);
    EXPECT(L, t, T_RPAREN, "')'");
    return 0;
}

static int str_eq_nocase(const char *a, const char *b) {
    unsigned char ca;
    unsigned char cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (tolower(ca) != tolower(cb)) return 0;
    }
    return *a == 0 && *b == 0;
}

static int env_truthy(const char *name) {
    const char *v = getenv(name);
    if (!v || *v == 0) return 0;
    return str_eq_nocase(v, "1") ||
           str_eq_nocase(v, "true") ||
           str_eq_nocase(v, "yes") ||
           str_eq_nocase(v, "on");
}

static tri_opt_config runtime_opt_config(void) {
    tri_opt_config cfg = {0};
    cfg.enable_experimental_transforms = env_truthy("TRINARY_OPT_EXPERIMENTAL");
    cfg.allow_observable_output_changes = env_truthy("TRINARY_OPT_ALLOW_OBSERVABLE");
    return cfg;
}

static int runtime_opt_trace_enabled(void) {
    return env_truthy("TRINARY_OPT_TRACE");
}

static void opt_report_accumulate(tri_opt_report *total, const tri_opt_report *part) {
    if (!total || !part) return;
    total->seen_insts += part->seen_insts;
    total->canonicalized_insts += part->canonicalized_insts;
    total->transformed_insts += part->transformed_insts;
    total->pass_runs += part->pass_runs;
    total->redundancy_candidates += part->redundancy_candidates;
}

static int flush_kernel_queue(
    tri_ir_program *queue,
    const tri_opt_config *cfg,
    int trace_enabled,
    tri_opt_report *total_report) {
    tri_opt_report report;
    if (!queue) return -1;
    if (queue->count == 0) return 0;
    if (tri_optimize_ir_program_with_config(queue, cfg, &report) != 0) return -1;
    if (trace_enabled) {
        fprintf(stderr,
                "[tri][opt] seen=%zu canonicalized=%zu transformed=%zu passes=%zu redundancy=%zu\n",
                report.seen_insts,
                report.canonicalized_insts,
                report.transformed_insts,
                report.pass_runs,
                report.redundancy_candidates);
    }
    opt_report_accumulate(total_report, &report);
    if (tri_ir_program_execute(queue) != 0) return -1;
    queue->count = 0;
    return 0;
}

static int parse_braincore(lexer_t *L, tri_ir_program *queue) {
    tok_t t;
    EXPECT(L, t, T_KW_BRAINCORE, "'braincore'");
    long long neurons = 4000000, iter = 1000;
    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_LPAREN) {
        tri_lex_advance(L, &t);
        EXPECT(L, t, T_INT, "neurons");
        neurons = t.ival;
        if (tri_lex_peek(L, &t) != 0) return -1;
        if (t.kind == T_COMMA) { tri_lex_advance(L, &t); EXPECT(L, t, T_INT, "iterations"); iter = t.ival; }
        EXPECT(L, t, T_RPAREN, "')'");
    }
    tri_ir_inst inst = {
        .kind = TRI_IR_BRAINCORE,
        .as.braincore = {
            .neurons = neurons,
            .iterations = iter
        }
    };
    return tri_ir_program_push(queue, inst);
}

static int parse_prime(lexer_t *L, tri_ir_program *queue) {
    tok_t t;
    EXPECT(L, t, T_KW_PRIME, "'prime'");
    long long limit = 1000000;
    if (tri_lex_peek(L, &t) != 0) return -1;
    if (t.kind == T_LPAREN) {
        tri_lex_advance(L, &t);
        EXPECT(L, t, T_INT, "limit");
        limit = t.ival;
        EXPECT(L, t, T_RPAREN, "')'");
    }
    tri_ir_inst inst = {
        .kind = TRI_IR_PRIME,
        .as.prime = {
            .limit = limit
        }
    };
    return tri_ir_program_push(queue, inst);
}

static int parse_stmt(
    lexer_t *L,
    tri_env_t *E,
    tri_ir_program *queue,
    const tri_opt_config *cfg,
    int trace_enabled,
    tri_opt_report *total_report) {
    tok_t t;
    if (tri_lex_peek(L, &t) != 0) return -1;
    switch (t.kind) {
        case T_KW_BRAINCORE: return parse_braincore(L, queue);
        case T_KW_PRIME: return parse_prime(L, queue);
        case T_KW_PRINT:
            if (flush_kernel_queue(queue, cfg, trace_enabled, total_report) != 0) return -1;
            return parse_print(L);
        case T_KW_PAR:
            if (flush_kernel_queue(queue, cfg, trace_enabled, total_report) != 0) return -1;
            return parse_par(L, E);
        case T_IDENT: {
            tok_t ident; tri_lex_advance(L, &ident);
            if (flush_kernel_queue(queue, cfg, trace_enabled, total_report) != 0) return -1;
            if (tri_lex_peek(L, &t) != 0) return -1;
            if (t.kind == T_COLON) return parse_decl(L, E, ident);
            if (t.kind == T_EQUALS) return parse_assignment(L, E, ident, 1);
            fprintf(stderr, "[tri] parse error at %d:%d: bare identifier '%.*s'\n",
                    ident.line, ident.col, (int)ident.len, ident.start);
            return -1;
        }
        default:
            fprintf(stderr, "[tri] parse error at %d:%d: unexpected token\n", t.line, t.col);
            return -1;
    }
}

static int parse_program(lexer_t *L, tri_env_t *E) {
    tok_t t;
    int rc = -1;
    tri_ir_program queue;
    tri_opt_config cfg = runtime_opt_config();
    tri_opt_report total_report = {0};
    int trace_enabled = runtime_opt_trace_enabled();
    tri_ir_program_init(&queue);
    while (1) {
        if (tri_lex_peek(L, &t) != 0) goto out;
        if (t.kind == T_EOF) {
            rc = flush_kernel_queue(&queue, &cfg, trace_enabled, &total_report);
            goto out;
        }
        if (t.kind == T_NEWLINE || t.kind == T_DEDENT || t.kind == T_INDENT) {
            tri_lex_advance(L, &t);
            continue;
        }
        if (parse_stmt(L, E, &queue, &cfg, trace_enabled, &total_report) != 0) goto out;
    }
out:
    if (trace_enabled && rc == 0) {
        fprintf(stderr,
                "[tri][opt-total] seen=%zu canonicalized=%zu transformed=%zu passes=%zu redundancy=%zu\n",
                total_report.seen_insts,
                total_report.canonicalized_insts,
                total_report.transformed_insts,
                total_report.pass_runs,
                total_report.redundancy_candidates);
    }
    tri_ir_program_free(&queue);
    return rc;
}

trinary_v1_status trinary_v1_run_source(const char *source, size_t len) {
    if (!source) return TRINARY_ERR_ARGS;
    if (len == 0) len = strlen(source);
    trinary_v1_init();
    lexer_t L;
    tri_lex_init(&L, source, len);
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
