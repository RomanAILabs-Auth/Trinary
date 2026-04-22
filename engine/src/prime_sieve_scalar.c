/*
 * prime_sieve_scalar.c — odd-only segmented sieve (bitset optimized).
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * This stays scalar but uses a packed bitset (1 bit per odd candidate) and
 * popcount-based counting. It is significantly faster and denser than the
 * byte-per-candidate reference while preserving deterministic behavior.
 */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
  #include <intrin.h>
#endif

static uint64_t isqrt_u64(uint64_t x) {
    return (uint64_t)sqrt((double)x);
}

static uint32_t popcnt64(uint64_t x) {
#if defined(_MSC_VER)
    return (uint32_t)__popcnt64(x);
#else
    return (uint32_t)__builtin_popcountll(x);
#endif
}

uint64_t trinary_v1_prime_sieve_u64_scalar(uint64_t limit) {
    if (limit < 2) return 0;
    if (limit == 2) return 1;

    const uint64_t root = isqrt_u64(limit);
    const uint64_t base_odd_count = (root >= 3) ? ((root - 3) / 2 + 1) : 0;
    const uint64_t base_words = (base_odd_count + 63) / 64;

    uint64_t *base_bits = NULL;
    if (base_words > 0) {
        base_bits = (uint64_t *)calloc((size_t)base_words, sizeof(uint64_t));
        if (!base_bits) return 0;
    }

    /* Store odd primes only (>=3). */
    uint64_t *primes = (uint64_t *)malloc((size_t)(base_odd_count + 1) * sizeof(uint64_t));
    if (!primes) {
        free(base_bits);
        return 0;
    }
    uint64_t prime_n = 0;

    for (uint64_t i = 0; i < base_odd_count; ++i) {
        if (base_bits[i >> 6] & (1ull << (i & 63))) continue;
        const uint64_t p = 2 * i + 3;
        primes[prime_n++] = p;

        if (p > root / p) continue;
        for (uint64_t j = ((p * p) - 3) / 2; j < base_odd_count; j += p) {
            base_bits[j >> 6] |= 1ull << (j & 63);
        }
    }
    free(base_bits);

    /* 2M odd numbers ~= 4M integer span per segment. */
    const uint64_t segment_odds = 1u << 21;
    const uint64_t seg_words = (segment_odds + 63) / 64;
    uint64_t *seg_bits = (uint64_t *)malloc((size_t)seg_words * sizeof(uint64_t));
    if (!seg_bits) {
        free(primes);
        return 0;
    }

    uint64_t count = 1; /* prime 2 */
    uint64_t low = 3;

    while (low <= limit) {
        uint64_t high = low + 2 * segment_odds - 2;
        if (high > limit) high = limit;
        if ((high & 1u) == 0) high--;
        if (high < low) break;

        const uint64_t odd_count = (high - low) / 2 + 1;
        const uint64_t word_count = (odd_count + 63) / 64;
        memset(seg_bits, 0, (size_t)word_count * sizeof(uint64_t));

        for (uint64_t pi = 0; pi < prime_n; ++pi) {
            const uint64_t p = primes[pi];
            if (p > high / p) break;

            uint64_t start = p * p;
            if (start < low) {
                start = ((low + p - 1) / p) * p;
            }
            if ((start & 1u) == 0) start += p;
            if (start > high) continue;

            for (uint64_t n = start; n <= high; n += 2 * p) {
                const uint64_t idx = (n - low) >> 1;
                seg_bits[idx >> 6] |= 1ull << (idx & 63);
            }
        }

        uint64_t composite = 0;
        for (uint64_t w = 0; w < word_count; ++w) {
            composite += popcnt64(seg_bits[w]);
        }
        if ((odd_count & 63) != 0) {
            const uint64_t used = odd_count & 63;
            const uint64_t tail_mask = ~((1ull << used) - 1ull);
            composite -= popcnt64(seg_bits[word_count - 1] & tail_mask);
        }

        count += odd_count - composite;

        if (high >= limit - 2) break;
        low = high + 2;
    }

    free(seg_bits);
    free(primes);
    return count;
}
