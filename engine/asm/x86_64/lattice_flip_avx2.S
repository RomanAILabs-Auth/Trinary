/* ============================================================================
 * lattice_flip_avx2.S — Trinary v1 bit-packed lattice flip (XOR with mask).
 * GAS Intel-syntax assembly, x86_64 Microsoft ABI (Windows).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * C signature:
 *   void trinary_v1_lattice_flip_avx2(
 *        uint64_t *lattice,     // RCX
 *        size_t    word_count,  // RDX  (multiple of 4)
 *        size_t    iterations,  // R8
 *        uint64_t  mask);       // R9
 * ========================================================================== */
.intel_syntax noprefix
.text
.globl trinary_v1_lattice_flip_avx2

trinary_v1_lattice_flip_avx2:
    test    r8, r8
    jz      lf_done
    test    rdx, rdx
    jz      lf_done

    vmovq   xmm3, r9
    vpbroadcastq ymm3, xmm3

    mov     r10, r8
    shl     rdx, 3
lf_iter:
    xor     r11, r11
lf_inner:
    cmp     r11, rdx
    jae     lf_next
    vmovdqu ymm0, ymmword ptr [rcx + r11]
    vpxor   ymm0, ymm0, ymm3
    vmovdqu ymmword ptr [rcx + r11], ymm0
    add     r11, 32
    jmp     lf_inner
lf_next:
    dec     r10
    jnz     lf_iter
lf_done:
    vzeroupper
    ret

.section .drectve
.ascii " -export:trinary_v1_lattice_flip_avx2"
