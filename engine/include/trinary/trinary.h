/*
 * trinary.h — Trinary Engine v1 Public C ABI
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * Stable C ABI. All symbols are prefixed `trinary_v1_`.
 * Any breaking change requires a major version bump and a new prefix (`trinary_v2_`).
 */
#ifndef TRINARY_V1_H_
#define TRINARY_V1_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #if defined(TRINARY_BUILD_SHARED)
    #define TRI_API __declspec(dllexport)
  #elif defined(TRINARY_USE_SHARED)
    #define TRI_API __declspec(dllimport)
  #else
    #define TRI_API
  #endif
#else
  #define TRI_API __attribute__((visibility("default")))
#endif

#define TRINARY_VERSION_MAJOR 0
#define TRINARY_VERSION_MINOR 1
#define TRINARY_VERSION_PATCH 0
#define TRINARY_VERSION_STRING "0.1.0"

/* =========================================================================
 * Status codes
 * ========================================================================= */
typedef enum trinary_v1_status {
    TRINARY_OK            = 0,
    TRINARY_ERR_ARGS      = 1,
    TRINARY_ERR_ALLOC     = 2,
    TRINARY_ERR_CPU       = 3,  /* required CPU feature missing */
    TRINARY_ERR_IO        = 4,
    TRINARY_ERR_PARSE     = 5,
    TRINARY_ERR_TYPE      = 6,
    TRINARY_ERR_INTERNAL  = 99
} trinary_v1_status;

/* =========================================================================
 * CPU feature bits (returned by trinary_v1_cpu_features)
 * ========================================================================= */
#define TRINARY_CPU_SSE2    (1u << 0)
#define TRINARY_CPU_SSE42   (1u << 1)
#define TRINARY_CPU_AVX     (1u << 2)
#define TRINARY_CPU_AVX2    (1u << 3)
#define TRINARY_CPU_AVX512F (1u << 4)
#define TRINARY_CPU_BMI2    (1u << 5)
#define TRINARY_CPU_POPCNT  (1u << 6)
#define TRINARY_CPU_FMA     (1u << 7)

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

/** Initialize the engine. Safe to call multiple times. Returns TRINARY_OK. */
TRI_API trinary_v1_status trinary_v1_init(void);

/** Human-readable build string: "trinary 0.1.0 (msvc 19.44, x86_64, avx2)". */
TRI_API const char *trinary_v1_version(void);

/** Bitmask of detected CPU features. */
TRI_API uint32_t trinary_v1_cpu_features(void);

/** Name of the best-available kernel variant for a given kernel id. */
TRI_API const char *trinary_v1_active_variant(const char *kernel_name);

/* =========================================================================
 * Memory
 * ========================================================================= */

/** Aligned allocation. Returns NULL on failure. Alignment must be power-of-two >= 64. */
TRI_API void *trinary_v1_alloc(size_t bytes, size_t alignment);

/** Free memory returned by trinary_v1_alloc. NULL-safe. */
TRI_API void trinary_v1_free(void *ptr);

/** Seed the internal PRNG. If seed==0, uses a high-quality OS source. */
TRI_API void trinary_v1_rng_seed(uint64_t seed);

/** Fill buffer with pseudo-random bytes (PCG64). */
TRI_API void trinary_v1_rng_fill(void *buf, size_t bytes);

/* =========================================================================
 * Timing
 * ========================================================================= */

/** Monotonic high-resolution timestamp in seconds since an arbitrary epoch. */
TRI_API double trinary_v1_now_seconds(void);

/* =========================================================================
 * Kernel: braincore — 8-bit LIF neuromorphic lattice
 *
 * For each iteration, for each neuron i:
 *    potentials[i] = saturating_add_u8(potentials[i], inputs[i]);
 *    if (potentials[i] >= threshold) potentials[i] = 0;   // spike + reset
 *
 * Buffers must be 32-byte aligned. `count` must be a multiple of 32.
 * ========================================================================= */
TRI_API trinary_v1_status trinary_v1_braincore_u8(
    uint8_t       *potentials,
    const uint8_t *inputs,
    size_t         count,
    size_t         iterations,
    uint8_t        threshold);

/* =========================================================================
 * Kernel: harding_gate — (a*b) - (a^b) over int16 arrays.
 *
 * out[i] = (a[i] * b[i]) - (a[i] ^ b[i])
 *
 * Buffers must be 32-byte aligned. `count` must be a multiple of 16.
 * ========================================================================= */
TRI_API trinary_v1_status trinary_v1_harding_gate_i16(
    int16_t       *out,
    const int16_t *a,
    const int16_t *b,
    size_t         count);

/* =========================================================================
 * Kernel: lattice_flip — XOR each 64-bit lane with mask (bit-packed flip).
 *
 * Interprets lattice as bit-packed trit sign flags. `iterations` rounds of flip.
 * Buffer must be 32-byte aligned. `word_count` must be a multiple of 4 (256 bits).
 * ========================================================================= */
TRI_API trinary_v1_status trinary_v1_lattice_flip(
    uint64_t      *lattice,
    size_t         word_count,
    size_t         iterations,
    uint64_t       mask);

/* =========================================================================
 * Kernel: rotor_cl4 — reference 4D rotor transform on packed vec4<f32>.
 *
 * Applies a two-plane rotor in XY and ZW planes:
 *   [x', y'] = R(theta_xy) * [x, y]
 *   [z', w'] = R(theta_zw) * [z, w]
 *
 * Input/output are packed as [x0,y0,z0,w0, x1,y1,z1,w1, ...].
 * Buffers must be 32-byte aligned. `vec4_count` is number of 4D vectors.
 * ========================================================================= */
TRI_API trinary_v1_status trinary_v1_rotor_cl4_f32(
    float         *out_xyzw,
    const float   *in_xyzw,
    size_t         vec4_count,
    float          theta_xy,
    float          theta_zw);

/* =========================================================================
 * Kernel: prime_sieve — count primes <= limit.
 *
 * Scalar reference uses an odd-only segmented sieve. AVX-512 popcount path is
 * planned for a future variant behind the same ABI surface.
 * ========================================================================= */
TRI_API trinary_v1_status trinary_v1_prime_sieve_u64(
    uint64_t       limit,
    uint64_t      *prime_count);

/* =========================================================================
 * Compiler: .tri -> machine code (v1 interpreter-dispatch backend)
 * ========================================================================= */

/** Run a .tri file. Returns process exit code analog (0 = success). */
TRI_API trinary_v1_status trinary_v1_run_file(const char *path);

/** Run a .tri source buffer. `source` need not be NUL-terminated if len > 0. */
TRI_API trinary_v1_status trinary_v1_run_source(const char *source, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TRINARY_V1_H_ */
