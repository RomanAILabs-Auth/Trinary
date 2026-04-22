/* ============================================================================
 * braincore_u8_avx2.S — Trinary v1 8-bit LIF neuromorphic lattice kernel.
 * GAS Intel-syntax assembly, x86_64 Microsoft ABI (Windows).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * C signature:
 *   void trinary_v1_braincore_u8_avx2(
 *        uint8_t       *potentials,  // RCX
 *        const uint8_t *inputs,      // RDX
 *        size_t         count,       // R8   (multiple of 32)
 *        size_t         iterations,  // R9
 *        uint8_t        threshold);  // [RSP+40]
 *
 * Per iteration, per 32-byte block:
 *   v_pot  = vpaddusb(v_pot, v_in)            // saturating u8 add
 *   mask   = (v_pot >= thresh)
 *   v_pot  = ~mask & v_pot                    // reset on spike
 * ========================================================================== */
.intel_syntax noprefix
.text
.globl trinary_v1_braincore_u8_avx2

trinary_v1_braincore_u8_avx2:
    test    r9, r9
    jz      bc_done
    test    r8, r8
    jz      bc_done

    movzx   eax, byte ptr [rsp + 40]
    vmovd   xmm3, eax
    vpbroadcastb ymm3, xmm3

    mov     r10, r9
bc_iter:
    xor     r11, r11
bc_inner:
    cmp     r11, r8
    jae     bc_next
    vmovdqu ymm0, ymmword ptr [rcx + r11]
    vmovdqu ymm1, ymmword ptr [rdx + r11]
    vpaddusb ymm0, ymm0, ymm1
    vpmaxub ymm2, ymm0, ymm3
    vpcmpeqb ymm2, ymm2, ymm0
    vpandn  ymm0, ymm2, ymm0
    vmovdqu ymmword ptr [rcx + r11], ymm0
    add     r11, 32
    jmp     bc_inner
bc_next:
    dec     r10
    jnz     bc_iter
bc_done:
    vzeroupper
    ret

.section .drectve
.ascii " -export:trinary_v1_braincore_u8_avx2"
