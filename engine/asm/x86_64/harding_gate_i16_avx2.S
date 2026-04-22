/* ============================================================================
 * harding_gate_i16_avx2.S — Trinary v1 Harding-Gate kernel over int16 arrays.
 * GAS Intel-syntax assembly, x86_64 Microsoft ABI (Windows).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * C signature:
 *   void trinary_v1_harding_gate_i16_avx2(
 *        int16_t       *out,     // RCX
 *        const int16_t *a,       // RDX
 *        const int16_t *b,       // R8
 *        size_t         count);  // R9  (number of int16, multiple of 16)
 *
 *   out[i] = (a[i] * b[i]) - (a[i] ^ b[i])
 * ========================================================================== */
.intel_syntax noprefix
.text
.globl trinary_v1_harding_gate_i16_avx2

trinary_v1_harding_gate_i16_avx2:
    test    r9, r9
    jz      hg_done
    xor     r10, r10
    shl     r9, 1
hg_loop:
    cmp     r10, r9
    jae     hg_done
    vmovdqu ymm0, ymmword ptr [rdx + r10]
    vmovdqu ymm1, ymmword ptr [r8 + r10]
    vpmullw ymm2, ymm0, ymm1
    vpxor   ymm3, ymm0, ymm1
    vpsubw  ymm2, ymm2, ymm3
    vmovdqu ymmword ptr [rcx + r10], ymm2
    add     r10, 32
    jmp     hg_loop
hg_done:
    vzeroupper
    ret

.section .drectve
.ascii " -export:trinary_v1_harding_gate_i16_avx2"
