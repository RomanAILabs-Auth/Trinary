/*
 * capi.c — Public C ABI thin wrappers with argument validation.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#include <stdint.h>

typedef void (*braincore_fn)(uint8_t *, const uint8_t *, size_t, size_t, uint8_t);
typedef void (*harding_fn)(int16_t *, const int16_t *, const int16_t *, size_t);
typedef void (*flip_fn)(uint64_t *, size_t, size_t, uint64_t);
typedef void (*rotor_fn)(float *, const float *, size_t, float, float);
typedef uint64_t (*prime_fn)(uint64_t);

braincore_fn tri_dispatch_braincore(void);
harding_fn   tri_dispatch_harding(void);
flip_fn      tri_dispatch_flip(void);
rotor_fn     tri_dispatch_rotor(void);
prime_fn     tri_dispatch_prime(void);

static int is_aligned(const void *p, size_t a) {
    return ((uintptr_t)p & (a - 1)) == 0;
}

trinary_v1_status trinary_v1_braincore_u8(
    uint8_t *potentials, const uint8_t *inputs,
    size_t count, size_t iterations, uint8_t threshold)
{
    if (!potentials || !inputs)           return TRINARY_ERR_ARGS;
    if (count == 0 || iterations == 0)    return TRINARY_OK;
    if (count % 32 != 0)                  return TRINARY_ERR_ARGS;
    if (!is_aligned(potentials, 32) ||
        !is_aligned(inputs, 32))          return TRINARY_ERR_ARGS;
    tri_dispatch_braincore()(potentials, inputs, count, iterations, threshold);
    return TRINARY_OK;
}

trinary_v1_status trinary_v1_harding_gate_i16(
    int16_t *out, const int16_t *a, const int16_t *b, size_t count)
{
    if (!out || !a || !b)                 return TRINARY_ERR_ARGS;
    if (count == 0)                       return TRINARY_OK;
    if (count % 16 != 0)                  return TRINARY_ERR_ARGS;
    if (!is_aligned(out, 32) ||
        !is_aligned(a,   32) ||
        !is_aligned(b,   32))             return TRINARY_ERR_ARGS;
    tri_dispatch_harding()(out, a, b, count);
    return TRINARY_OK;
}

trinary_v1_status trinary_v1_lattice_flip(
    uint64_t *lattice, size_t word_count, size_t iterations, uint64_t mask)
{
    if (!lattice)                         return TRINARY_ERR_ARGS;
    if (word_count == 0 || iterations == 0) return TRINARY_OK;
    if (word_count % 4 != 0)              return TRINARY_ERR_ARGS;
    if (!is_aligned(lattice, 32))         return TRINARY_ERR_ARGS;
    tri_dispatch_flip()(lattice, word_count, iterations, mask);
    return TRINARY_OK;
}

trinary_v1_status trinary_v1_rotor_cl4_f32(
    float *out_xyzw, const float *in_xyzw, size_t vec4_count, float theta_xy, float theta_zw)
{
    if (!out_xyzw || !in_xyzw)            return TRINARY_ERR_ARGS;
    if (vec4_count == 0)                  return TRINARY_OK;
    if (!is_aligned(out_xyzw, 32) ||
        !is_aligned(in_xyzw,  32))        return TRINARY_ERR_ARGS;
    tri_dispatch_rotor()(out_xyzw, in_xyzw, vec4_count, theta_xy, theta_zw);
    return TRINARY_OK;
}

trinary_v1_status trinary_v1_prime_sieve_u64(uint64_t limit, uint64_t *prime_count)
{
    if (!prime_count) return TRINARY_ERR_ARGS;
    *prime_count = tri_dispatch_prime()(limit);
    return TRINARY_OK;
}
