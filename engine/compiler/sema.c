/*
 * sema.c — v0 semantic checks for the .tri front-end.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * This is intentionally lightweight in v0: it enforces declaration extents,
 * loop bounds sanity, and argument canonicalization for kernel statements.
 */
#include "sema.h"

#include <stdio.h>

int tri_sema_validate_trit_decl(long long n, int line, int col) {
    if (n <= 0) {
        fprintf(stderr, "[tri] sema error at %d:%d: trit extent must be > 0\n", line, col);
        return -1;
    }
    return 0;
}

int tri_sema_validate_cl4_decl(long long n, int line, int col) {
    if (n <= 0) {
        fprintf(stderr, "[tri] sema error at %d:%d: cl4 extent must be > 0\n", line, col);
        return -1;
    }
    return 0;
}

int tri_sema_validate_loop_range(long long lo, long long hi, int line, int col) {
    if (hi < lo) {
        fprintf(stderr, "[tri] sema error at %d:%d: range stop must be >= start\n", line, col);
        return -1;
    }
    return 0;
}

void tri_sema_finalize_braincore(long long *neurons, long long *iters) {
    if (!neurons || !iters) return;
    if (*neurons <= 0) *neurons = 4000000;
    if (*iters <= 0) *iters = 1000;
    if ((*neurons % 32) != 0) {
        *neurons = ((*neurons + 31) / 32) * 32;
        if (*neurons <= 0) *neurons = 32;
    }
}

void tri_sema_finalize_prime(long long *limit) {
    if (!limit) return;
    if (*limit < 0) *limit = 0;
}
