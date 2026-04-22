/*
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#ifndef TRINARY_TRI_FRONTEND_H_
#define TRINARY_TRI_FRONTEND_H_

#include <stddef.h>

typedef enum {
    T_EOF, T_NEWLINE, T_INDENT, T_DEDENT,
    T_IDENT, T_INT, T_STRING,
    T_COLON, T_LPAREN, T_RPAREN, T_LBRACK, T_RBRACK,
    T_COMMA, T_EQUALS, T_STAR, T_MINUS, T_DOT,
    T_KW_PRINT, T_KW_PAR, T_KW_FOR, T_KW_IN, T_KW_RANGE,
    T_KW_TRIT, T_KW_CL4, T_KW_SPACETIME, T_KW_BRAINCORE, T_KW_PRIME, T_KW_SOA
} tok_kind;

typedef struct {
    tok_kind kind;
    const char *start;
    size_t len;
    long long ival;
    int line, col;
    int indent;
} tok_t;

typedef struct {
    const char *src;
    size_t len;
    size_t pos;
    int line, col;
    int indent_stack[64];
    int indent_depth;
    int pending_dedents;
    int at_line_start;
    tok_t lookahead;
    int has_lookahead;
} lexer_t;

void tri_lex_init(lexer_t *L, const char *src, size_t len);
int tri_lex_peek(lexer_t *L, tok_t *out);
int tri_lex_advance(lexer_t *L, tok_t *out);

#endif
