/*
 * dispatch.c — Load-time kernel dispatch.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * trinary_v1_init() runs CPUID, picks the best-available implementation for
 * each kernel slot, and stores function pointers in the global dispatch table.
 * Hot loops never re-check features.
 */
#include "trinary/trinary.h"

#include <stdint.h>
#include <string.h>

/* External declarations — the asm variants (Windows x64 only for now). */
#if defined(_WIN32)
extern void trinary_v1_braincore_u8_avx2(
    uint8_t *pot, const uint8_t *in, size_t count, size_t iter, uint8_t thresh);
extern void trinary_v1_harding_gate_i16_avx2(
    int16_t *out, const int16_t *a, const int16_t *b, size_t count);
extern void trinary_v1_lattice_flip_avx2(
    uint64_t *lattice, size_t word_count, size_t iter, uint64_t mask);
#endif

/* Scalar fallbacks. */
extern void trinary_v1_braincore_u8_scalar(
    uint8_t *pot, const uint8_t *in, size_t count, size_t iter, uint8_t thresh);
extern void trinary_v1_harding_gate_i16_scalar(
    int16_t *out, const int16_t *a, const int16_t *b, size_t count);
extern void trinary_v1_lattice_flip_scalar(
    uint64_t *lattice, size_t word_count, size_t iter, uint64_t mask);
extern void trinary_v1_rotor_cl4_f32_scalar(
    float *out_xyzw, const float *in_xyzw, size_t vec4_count, float theta_xy, float theta_zw);
extern void trinary_v1_rotor_cl4_f32_avx2(
    float *out_xyzw, const float *in_xyzw, size_t vec4_count, float theta_xy, float theta_zw);
extern uint64_t trinary_v1_prime_sieve_u64_scalar(uint64_t limit);

/* Function pointer types. */
typedef void (*braincore_fn)(uint8_t *, const uint8_t *, size_t, size_t, uint8_t);
typedef void (*harding_fn)(int16_t *, const int16_t *, const int16_t *, size_t);
typedef void (*flip_fn)(uint64_t *, size_t, size_t, uint64_t);
typedef void (*rotor_fn)(float *, const float *, size_t, float, float);
typedef uint64_t (*prime_fn)(uint64_t);

/* The dispatch table. */
typedef struct {
    braincore_fn braincore;
    harding_fn   harding;
    flip_fn      flip;
    rotor_fn     rotor;
    prime_fn     prime;

    const char  *braincore_variant;
    const char  *harding_variant;
    const char  *flip_variant;
    const char  *rotor_variant;
    const char  *prime_variant;

    int initialized;
} tri_dispatch_t;

static tri_dispatch_t g_dispatch = {0};

trinary_v1_status trinary_v1_init(void) {
    if (g_dispatch.initialized) return TRINARY_OK;

    uint32_t feats = trinary_v1_cpu_features();

    /* Defaults: scalar. */
    g_dispatch.braincore         = trinary_v1_braincore_u8_scalar;
    g_dispatch.braincore_variant = "scalar";
    g_dispatch.harding           = trinary_v1_harding_gate_i16_scalar;
    g_dispatch.harding_variant   = "scalar";
    g_dispatch.flip              = trinary_v1_lattice_flip_scalar;
    g_dispatch.flip_variant      = "scalar";
    g_dispatch.rotor             = trinary_v1_rotor_cl4_f32_scalar;
    g_dispatch.rotor_variant     = "scalar";
    g_dispatch.prime             = trinary_v1_prime_sieve_u64_scalar;
    g_dispatch.prime_variant     = "scalar";

#if defined(_WIN32)
    if (feats & TRINARY_CPU_AVX2) {
        g_dispatch.braincore         = trinary_v1_braincore_u8_avx2;
        g_dispatch.braincore_variant = "avx2";
        g_dispatch.harding           = trinary_v1_harding_gate_i16_avx2;
        g_dispatch.harding_variant   = "avx2";
        g_dispatch.flip              = trinary_v1_lattice_flip_avx2;
        g_dispatch.flip_variant      = "avx2";
        g_dispatch.rotor             = trinary_v1_rotor_cl4_f32_avx2;
        g_dispatch.rotor_variant     = "avx2";
    }
#endif

    g_dispatch.initialized = 1;
    return TRINARY_OK;
}

const char *trinary_v1_active_variant(const char *kernel_name) {
    if (!g_dispatch.initialized) trinary_v1_init();
    if (!kernel_name) return "";
    if (strcmp(kernel_name, "braincore") == 0) return g_dispatch.braincore_variant;
    if (strcmp(kernel_name, "harding")   == 0) return g_dispatch.harding_variant;
    if (strcmp(kernel_name, "flip")      == 0) return g_dispatch.flip_variant;
    if (strcmp(kernel_name, "rotor")     == 0) return g_dispatch.rotor_variant;
    if (strcmp(kernel_name, "prime")     == 0) return g_dispatch.prime_variant;
    return "unknown";
}

/* Internal accessors used by capi.c. */
braincore_fn tri_dispatch_braincore(void) {
    if (!g_dispatch.initialized) trinary_v1_init();
    return g_dispatch.braincore;
}
harding_fn tri_dispatch_harding(void) {
    if (!g_dispatch.initialized) trinary_v1_init();
    return g_dispatch.harding;
}
flip_fn tri_dispatch_flip(void) {
    if (!g_dispatch.initialized) trinary_v1_init();
    return g_dispatch.flip;
}
rotor_fn tri_dispatch_rotor(void) {
    if (!g_dispatch.initialized) trinary_v1_init();
    return g_dispatch.rotor;
}
prime_fn tri_dispatch_prime(void) {
    if (!g_dispatch.initialized) trinary_v1_init();
    return g_dispatch.prime;
}
