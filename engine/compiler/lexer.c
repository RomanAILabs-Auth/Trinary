/*
 * lexer.c — Trinary v0 lexer (extracted from tri_lang.c).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "tri_frontend.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int lex_next(lexer_t *L, tok_t *t);

static int lex_get(lexer_t *L) {
    if (L->pos >= L->len) return -1;
    int c = (unsigned char)L->src[L->pos++];
    if (c == '\n') { L->line++; L->col = 1; } else { L->col++; }
    return c;
}

void tri_lex_init(lexer_t *L, const char *src, size_t len) {
    memset(L, 0, sizeof(*L));
    L->src = src; L->len = len; L->line = 1; L->col = 1;
    L->at_line_start = 1;
    L->indent_stack[0] = 0;
    L->indent_depth = 1;
}

static tok_kind classify_ident(const char *s, size_t n) {
    /* Length/leading-char dispatch avoids repeated strlen/memcmp chains. */
    switch (n) {
        case 2:
            if (s[0] == 'i' && s[1] == 'n') return T_KW_IN;
            break;
        case 3:
            if (s[0] == 'p' && s[1] == 'a' && s[2] == 'r') return T_KW_PAR;
            if (s[0] == 'f' && s[1] == 'o' && s[2] == 'r') return T_KW_FOR;
            if (s[0] == 's' && s[1] == 'o' && s[2] == 'a') return T_KW_SOA;
            break;
        case 4:
            if (s[0] == 't' && s[1] == 'r' && s[2] == 'i' && s[3] == 't') return T_KW_TRIT;
            if (s[0] == 'c' && s[1] == 'l' && s[2] == '4') return T_KW_CL4;
            break;
        case 5:
            if (s[0] == 'p' && memcmp(s, "print", 5) == 0) return T_KW_PRINT;
            if (s[0] == 'r' && memcmp(s, "range", 5) == 0) return T_KW_RANGE;
            if (s[0] == 'p' && memcmp(s, "prime", 5) == 0) return T_KW_PRIME;
            break;
        case 9:
            if (s[0] == 'b' && memcmp(s, "braincore", 9) == 0) return T_KW_BRAINCORE;
            if (s[0] == 's' && memcmp(s, "spacetime", 9) == 0) return T_KW_SPACETIME;
            break;
        default:
            break;
    }
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

    if (L->at_line_start) {
        int spaces = 0;
        while (L->pos < L->len) {
            int c = (unsigned char)L->src[L->pos];
            if (c == ' ')      { spaces++; L->pos++; L->col++; }
            else if (c == '\t'){ spaces += 4; L->pos++; L->col++; }
            else break;
        }
        int c0 = (L->pos < L->len) ? (unsigned char)L->src[L->pos] : -1;
        if (c0 == '\n' || c0 == '#' || c0 == -1) {
            L->at_line_start = 0;
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

    while (L->pos < L->len) {
        int c = (unsigned char)L->src[L->pos];
        if (c == ' ' || c == '\t' || c == '\r') { L->pos++; L->col++; continue; }
        break;
    }

    if (L->pos >= L->len) {
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

int tri_lex_peek(lexer_t *L, tok_t *out) {
    if (L->has_lookahead) { *out = L->lookahead; return 0; }
    int r = lex_next(L, &L->lookahead);
    if (r != 0) return r;
    L->has_lookahead = 1;
    *out = L->lookahead;
    return 0;
}

int tri_lex_advance(lexer_t *L, tok_t *out) {
    if (L->has_lookahead) { *out = L->lookahead; L->has_lookahead = 0; return 0; }
    return lex_next(L, out);
}
