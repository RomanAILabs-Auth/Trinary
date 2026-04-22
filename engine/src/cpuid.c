/*
 * cpuid.c — CPU feature detection (Windows x64, intrin.h).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#include <intrin.h>
#include <stdint.h>
#include <string.h>

static uint32_t g_features = 0;
static int g_initialized = 0;

static void detect_features(void) {
    int info[4] = {0};

    __cpuid(info, 0);
    int max_leaf = info[0];

    if (max_leaf >= 1) {
        __cpuid(info, 1);
        uint32_t ecx1 = (uint32_t)info[2];
        uint32_t edx1 = (uint32_t)info[3];
        if (edx1 & (1u << 26)) g_features |= TRINARY_CPU_SSE2;
        if (ecx1 & (1u << 20)) g_features |= TRINARY_CPU_SSE42;
        if (ecx1 & (1u << 28)) g_features |= TRINARY_CPU_AVX;
        if (ecx1 & (1u << 23)) g_features |= TRINARY_CPU_POPCNT;
        if (ecx1 & (1u << 12)) g_features |= TRINARY_CPU_FMA;
    }

    if (max_leaf >= 7) {
        __cpuidex(info, 7, 0);
        uint32_t ebx7 = (uint32_t)info[1];
        if (ebx7 & (1u << 5))  g_features |= TRINARY_CPU_AVX2;
        if (ebx7 & (1u << 16)) g_features |= TRINARY_CPU_AVX512F;
        if (ebx7 & (1u << 8))  g_features |= TRINARY_CPU_BMI2;
    }

    /* AVX/AVX2/AVX512 also require OS XSAVE support. */
    if (g_features & (TRINARY_CPU_AVX | TRINARY_CPU_AVX2 | TRINARY_CPU_AVX512F)) {
        uint64_t xcr0 = _xgetbv(0);
        /* bit 1 = SSE state, bit 2 = AVX state */
        if ((xcr0 & 0x6) != 0x6) {
            g_features &= ~(TRINARY_CPU_AVX | TRINARY_CPU_AVX2 | TRINARY_CPU_AVX512F);
        }
        /* bits 5-7 = AVX-512 state */
        if ((xcr0 & 0xE0) != 0xE0) {
            g_features &= ~TRINARY_CPU_AVX512F;
        }
    }
}

uint32_t trinary_v1_cpu_features(void) {
    if (!g_initialized) {
        detect_features();
        g_initialized = 1;
    }
    return g_features;
}
