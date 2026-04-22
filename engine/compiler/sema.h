/*
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#ifndef TRINARY_TRI_SEMA_H_
#define TRINARY_TRI_SEMA_H_

int tri_sema_validate_trit_decl(long long n, int line, int col);
int tri_sema_validate_cl4_decl(long long n, int line, int col);
int tri_sema_validate_loop_range(long long lo, long long hi, int line, int col);

void tri_sema_finalize_braincore(long long *neurons, long long *iters);
void tri_sema_finalize_prime(long long *limit);

#endif
