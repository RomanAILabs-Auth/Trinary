/*
 * rng.c — PCG64 PRNG. Thread-local state, OS-seeded on init.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
  #include <bcrypt.h>
#else
  #include <sys/random.h>
#endif

#if defined(_MSC_VER)
  #define TRI_TLS __declspec(thread)
#else
  #define TRI_TLS _Thread_local
#endif

static TRI_TLS uint64_t g_state = 0x853c49e6748fea9bULL;
static TRI_TLS uint64_t g_inc   = 0xda3e39cb94b95bdbULL;
static TRI_TLS int g_seeded = 0;

static uint32_t pcg32(void) {
    uint64_t old = g_state;
    g_state = old * 6364136223846793005ULL + (g_inc | 1u);
    uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    uint32_t rot = (uint32_t)(old >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-(int32_t)rot) & 31));
}

static void os_seed(uint64_t *seed0, uint64_t *seed1) {
    uint8_t buf[16] = {0};
#if defined(_WIN32)
    BCRYPT_ALG_HANDLE h = NULL;
    if (BCryptOpenAlgorithmProvider(&h, BCRYPT_RNG_ALGORITHM, NULL, 0) == 0) {
        BCryptGenRandom(h, buf, sizeof(buf), 0);
        BCryptCloseAlgorithmProvider(h, 0);
    }
#else
    (void)getrandom(buf, sizeof(buf), 0);
#endif
    memcpy(seed0, buf,     8);
    memcpy(seed1, buf + 8, 8);
}

void trinary_v1_rng_seed(uint64_t seed) {
    if (seed == 0) {
        uint64_t a = 0, b = 0;
        os_seed(&a, &b);
        g_state = a ? a : 0x853c49e6748fea9bULL;
        g_inc   = (b << 1) | 1u;
    } else {
        g_state = seed;
        g_inc   = (seed ^ 0xda3e39cb94b95bdbULL) | 1u;
    }
    g_seeded = 1;
}

void trinary_v1_rng_fill(void *buf, size_t bytes) {
    if (!g_seeded) trinary_v1_rng_seed(0);
    uint8_t *p = (uint8_t *)buf;
    while (bytes >= 4) {
        uint32_t v = pcg32();
        memcpy(p, &v, 4);
        p += 4; bytes -= 4;
    }
    if (bytes) {
        uint32_t v = pcg32();
        memcpy(p, &v, bytes);
    }
}
