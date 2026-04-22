/*
 * bench_engine.c — reproducible multi-replicate microbenchmark harness.
 *
 * For each kernel: N warmup runs, then M measured runs. Emits a single JSON
 * document to stdout with per-kernel min / median / mean / stddev seconds,
 * working-set size, active variant, and derived throughput. Shape is frozen
 * so the perf-gate diff tool (benchmarks/perf_gate.py) can parse old and new
 * output interchangeably.
 *
 * Intentionally dependency-free — no Google Benchmark, no libc-heavy stats.
 */
#include "trinary/trinary.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef WARMUP
#define WARMUP 3
#endif
#ifndef REPLICATES
#define REPLICATES 10
#endif

typedef struct {
    double min_s, max_s, mean_s, median_s, stddev_s;
} stats_t;

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x < y) ? -1 : (x > y);
}

static stats_t summarize(double *samples, int n) {
    qsort(samples, (size_t)n, sizeof(double), cmp_double);
    stats_t s = {0};
    s.min_s = samples[0];
    s.max_s = samples[n - 1];
    double sum = 0;
    for (int i = 0; i < n; ++i) sum += samples[i];
    s.mean_s   = sum / n;
    s.median_s = (n & 1) ? samples[n / 2]
                         : 0.5 * (samples[n / 2 - 1] + samples[n / 2]);
    double var = 0;
    for (int i = 0; i < n; ++i) {
        double d = samples[i] - s.mean_s;
        var += d * d;
    }
    s.stddev_s = (n > 1) ? sqrt(var / (n - 1)) : 0.0;
    return s;
}

static void emit_stats(const char *name, const char *variant,
                       const char *unit_key, double unit_per_run,
                       stats_t s, int first) {
    double best_throughput = unit_per_run / s.min_s;
    if (!first) printf(",\n");
    printf("    {\n");
    printf("      \"name\": \"%s\",\n", name);
    printf("      \"variant\": \"%s\",\n", variant);
    printf("      \"replicates\": %d,\n", REPLICATES);
    printf("      \"warmup\": %d,\n", WARMUP);
    printf("      \"seconds\": { \"min\": %.6f, \"median\": %.6f, "
           "\"mean\": %.6f, \"max\": %.6f, \"stddev\": %.6f },\n",
           s.min_s, s.median_s, s.mean_s, s.max_s, s.stddev_s);
    printf("      \"%s_per_sec_best\": %.3f\n", unit_key, best_throughput);
    printf("    }");
}

/* ------------------------------------------------------------------ */

static double bench_braincore(size_t neurons, size_t iters) {
    uint8_t *pot = trinary_v1_alloc(neurons, 32);
    uint8_t *inp = trinary_v1_alloc(neurons, 32);
    trinary_v1_rng_seed(0xBEEFULL);
    trinary_v1_rng_fill(inp, neurons);
    for (size_t i = 0; i < neurons; ++i) inp[i] %= 50;
    memset(pot, 0, neurons);
    double t0 = trinary_v1_now_seconds();
    trinary_v1_braincore_u8(pot, inp, neurons, iters, 200);
    double t1 = trinary_v1_now_seconds();
    trinary_v1_free(pot);
    trinary_v1_free(inp);
    return t1 - t0;
}

static double bench_harding(size_t count, int iters) {
    int16_t *a = trinary_v1_alloc(count * sizeof(int16_t), 32);
    int16_t *b = trinary_v1_alloc(count * sizeof(int16_t), 32);
    int16_t *o = trinary_v1_alloc(count * sizeof(int16_t), 32);
    trinary_v1_rng_seed(0xCAFEULL);
    trinary_v1_rng_fill(a, count * sizeof(int16_t));
    trinary_v1_rng_fill(b, count * sizeof(int16_t));
    double t0 = trinary_v1_now_seconds();
    for (int i = 0; i < iters; ++i)
        trinary_v1_harding_gate_i16(o, a, b, count);
    double t1 = trinary_v1_now_seconds();
    trinary_v1_free(a); trinary_v1_free(b); trinary_v1_free(o);
    return t1 - t0;
}

static double bench_flip(size_t words, int iters) {
    uint64_t *x = trinary_v1_alloc(words * sizeof(uint64_t), 32);
    trinary_v1_rng_seed(0xF00DULL);
    trinary_v1_rng_fill(x, words * sizeof(uint64_t));
    double t0 = trinary_v1_now_seconds();
    trinary_v1_lattice_flip(x, words, (size_t)iters, 0xAAAAAAAAAAAAAAAAULL);
    double t1 = trinary_v1_now_seconds();
    trinary_v1_free(x);
    return t1 - t0;
}

static double bench_prime(uint64_t limit, int iters) {
    uint64_t out = 0;
    double t0 = trinary_v1_now_seconds();
    for (int i = 0; i < iters; ++i) {
        trinary_v1_prime_sieve_u64(limit, &out);
    }
    double t1 = trinary_v1_now_seconds();
    return t1 - t0;
}

/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    trinary_v1_init();
    uint32_t f = trinary_v1_cpu_features();

    const size_t BC_N    = 4000000;
    const size_t BC_ITER = 1000;
    const size_t HG_N    = 16u * 1024u * 1024u;
    const int    HG_ITER = 64;
    /* 64 MiB worth of uint64_t = 8 Mi words = 512 Mbit lattice. */
    const size_t LF_N    = 8u * 1024u * 1024u;
    const int    LF_ITER = 64;
    const uint64_t PS_LIMIT = 20000000ull;
    const int      PS_ITER  = 8;

    printf("{\n");
    printf("  \"schema\": \"trinary.bench.v1\",\n");
    printf("  \"version\": \"%s\",\n", trinary_v1_version());
    printf("  \"cpu_features\": \"sse2=%d avx=%d avx2=%d avx512f=%d bmi2=%d fma=%d\",\n",
           !!(f & TRINARY_CPU_SSE2), !!(f & TRINARY_CPU_AVX),
           !!(f & TRINARY_CPU_AVX2), !!(f & TRINARY_CPU_AVX512F),
           !!(f & TRINARY_CPU_BMI2), !!(f & TRINARY_CPU_FMA));
    printf("  \"results\": [\n");

    double samples[REPLICATES];

    /* braincore_u8 */
    for (int i = 0; i < WARMUP; ++i) (void)bench_braincore(BC_N, BC_ITER);
    for (int i = 0; i < REPLICATES; ++i) samples[i] = bench_braincore(BC_N, BC_ITER);
    emit_stats("braincore_u8", trinary_v1_active_variant("braincore"),
               "gneurons", (double)BC_N * (double)BC_ITER / 1e9,
               summarize(samples, REPLICATES), 1);

    /* harding_gate_i16 */
    for (int i = 0; i < WARMUP; ++i) (void)bench_harding(HG_N, HG_ITER);
    for (int i = 0; i < REPLICATES; ++i) samples[i] = bench_harding(HG_N, HG_ITER);
    emit_stats("harding_gate_i16", trinary_v1_active_variant("harding"),
               "gelems", (double)HG_N * (double)HG_ITER / 1e9,
               summarize(samples, REPLICATES), 0);

    /* lattice_flip — throughput reported in Giga bits/s (64 bits per word). */
    for (int i = 0; i < WARMUP; ++i) (void)bench_flip(LF_N, LF_ITER);
    for (int i = 0; i < REPLICATES; ++i) samples[i] = bench_flip(LF_N, LF_ITER);
    emit_stats("lattice_flip", trinary_v1_active_variant("flip"),
               "gbits",
               (double)LF_N * 64.0 * (double)LF_ITER / 1e9,
               summarize(samples, REPLICATES), 0);

    /* prime_sieve — throughput reported in million tested upper-bounds/s. */
    for (int i = 0; i < WARMUP; ++i) (void)bench_prime(PS_LIMIT, PS_ITER);
    for (int i = 0; i < REPLICATES; ++i) samples[i] = bench_prime(PS_LIMIT, PS_ITER);
    emit_stats("prime_sieve", trinary_v1_active_variant("prime"),
               "mlimits", ((double)PS_LIMIT * (double)PS_ITER) / 1e6,
               summarize(samples, REPLICATES), 0);

    printf("\n  ]\n");
    printf("}\n");
    return 0;
}
