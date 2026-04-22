/*
 * main.c — trinary.exe native CLI entry point.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * Usage:
 *   trinary <file.tri>       Run a .tri program.
 *   trinary braincore        Run the canonical 8-bit benchmark.
 *   trinary bench            Run the full benchmark suite.
 *   trinary --version        Print version string.
 *   trinary --features       Print detected CPU features.
 *   trinary --help           Print this help.
 */
#include "trinary/trinary.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(void) {
    fputs(
        "trinary — machine-code kernels for lattice/neuromorphic compute\n"
        "\n"
        "Usage:\n"
        "  trinary <file.tri>\n"
        "  trinary braincore [NEURONS] [ITERS]\n"
        "  trinary prime [LIMIT]\n"
        "  trinary bench\n"
        "  trinary --version\n"
        "  trinary --features\n"
        "  trinary --help\n"
        "\n"
        "Optimizer flags (optional, default-safe):\n"
        "  --opt-default            Disable optimizer toggles.\n"
        "  --opt-experimental       Enable experimental optimizer transforms.\n"
        "  --opt-allow-observable   Allow transforms that can alter output.\n"
        "  --opt-trace              Emit optimizer telemetry to stderr.\n",
        stdout);
}

static int parse_int(const char *s, long long *out) {
    if (!s || !*s) return -1;
    long long v = 0;
    while (*s) {
        if (*s < '0' || *s > '9') return -1;
        v = v * 10 + (*s - '0');
        s++;
    }
    *out = v; return 0;
}

static void set_opt_env(const char *name, const char *value) {
#if defined(_WIN32)
    (void)_putenv_s(name, value);
#else
    (void)setenv(name, value, 1);
#endif
}

static int apply_opt_flag(const char *arg) {
    if (strcmp(arg, "--opt-default") == 0) {
        set_opt_env("TRINARY_OPT_EXPERIMENTAL", "0");
        set_opt_env("TRINARY_OPT_ALLOW_OBSERVABLE", "0");
        set_opt_env("TRINARY_OPT_TRACE", "0");
        return 1;
    }
    if (strcmp(arg, "--opt-experimental") == 0) {
        set_opt_env("TRINARY_OPT_EXPERIMENTAL", "1");
        return 1;
    }
    if (strcmp(arg, "--opt-allow-observable") == 0) {
        set_opt_env("TRINARY_OPT_ALLOW_OBSERVABLE", "1");
        return 1;
    }
    if (strcmp(arg, "--opt-trace") == 0) {
        set_opt_env("TRINARY_OPT_TRACE", "1");
        return 1;
    }
    return 0;
}

static int cmd_braincore(long long neurons, long long iters) {
    if (neurons <= 0) neurons = 4000000;
    if (iters   <= 0) iters   = 1000;
    if (neurons % 32 != 0) neurons = (neurons + 31) / 32 * 32;

    uint8_t *pot = (uint8_t *)trinary_v1_alloc((size_t)neurons, 32);
    uint8_t *inp = (uint8_t *)trinary_v1_alloc((size_t)neurons, 32);
    if (!pot || !inp) { fputs("alloc failed\n", stderr); return 2; }

    trinary_v1_rng_seed(0);
    trinary_v1_rng_fill(inp, (size_t)neurons);
    for (long long i = 0; i < neurons; ++i) inp[i] %= 50;
    for (long long i = 0; i < neurons; ++i) pot[i] = 0;

    double t0 = trinary_v1_now_seconds();
    trinary_v1_status rc = trinary_v1_braincore_u8(
        pot, inp, (size_t)neurons, (size_t)iters, 200);
    double t1 = trinary_v1_now_seconds();

    trinary_v1_free(pot);
    trinary_v1_free(inp);
    if (rc != TRINARY_OK) { fputs("braincore failed\n", stderr); return 2; }

    double elapsed = t1 - t0;
    double ops = (double)neurons * (double)iters;
    printf("{\n");
    printf("  \"kernel\": \"braincore\",\n");
    printf("  \"variant\": \"%s\",\n", trinary_v1_active_variant("braincore"));
    printf("  \"neurons\": %lld,\n", neurons);
    printf("  \"iterations\": %lld,\n", iters);
    printf("  \"seconds\": %.6f,\n", elapsed);
    printf("  \"giga_neurons_per_sec\": %.3f\n", ops / elapsed / 1e9);
    printf("}\n");
    fflush(stdout);
    return 0;
}

static int cmd_bench(void) {
    printf("[\n");
    (void)cmd_braincore(4000000, 1000);
    printf(",\n");
    /* harding gate — 16M int16 x 1 pass */
    const size_t COUNT = 16 * 1024 * 1024;
    int16_t *a = (int16_t *)trinary_v1_alloc(COUNT * sizeof(int16_t), 32);
    int16_t *b = (int16_t *)trinary_v1_alloc(COUNT * sizeof(int16_t), 32);
    int16_t *o = (int16_t *)trinary_v1_alloc(COUNT * sizeof(int16_t), 32);
    if (!a || !b || !o) { fputs("alloc failed\n", stderr); return 2; }
    trinary_v1_rng_fill(a, COUNT * sizeof(int16_t));
    trinary_v1_rng_fill(b, COUNT * sizeof(int16_t));
    double t0 = trinary_v1_now_seconds();
    const int ITER = 64;
    for (int i = 0; i < ITER; ++i)
        trinary_v1_harding_gate_i16(o, a, b, COUNT);
    double t1 = trinary_v1_now_seconds();
    trinary_v1_free(a); trinary_v1_free(b); trinary_v1_free(o);
    double e = t1 - t0;
    double ops = (double)COUNT * ITER;
    printf("{\n");
    printf("  \"kernel\": \"harding_gate_i16\",\n");
    printf("  \"variant\": \"%s\",\n", trinary_v1_active_variant("harding"));
    printf("  \"elements\": %zu,\n", (size_t)COUNT);
    printf("  \"iterations\": %d,\n", ITER);
    printf("  \"seconds\": %.6f,\n", e);
    printf("  \"giga_elements_per_sec\": %.3f\n", ops / e / 1e9);
    printf("}\n]\n");
    return 0;
}

static int cmd_prime(long long limit) {
    if (limit < 0) limit = 0;
    uint64_t count = 0;
    double t0 = trinary_v1_now_seconds();
    trinary_v1_status rc = trinary_v1_prime_sieve_u64((uint64_t)limit, &count);
    double t1 = trinary_v1_now_seconds();
    if (rc != TRINARY_OK) {
        fputs("prime_sieve failed\n", stderr);
        return 2;
    }
    const double elapsed = t1 - t0;
    printf("{\n");
    printf("  \"kernel\": \"prime_sieve\",\n");
    printf("  \"variant\": \"%s\",\n", trinary_v1_active_variant("prime"));
    printf("  \"limit\": %lld,\n", limit);
    printf("  \"prime_count\": %llu,\n", (unsigned long long)count);
    printf("  \"seconds\": %.6f,\n", elapsed);
    printf("  \"million_numbers_per_sec\": %.3f\n", elapsed > 0.0 ? ((double)limit / elapsed / 1e6) : 0.0);
    printf("}\n");
    return 0;
}

int main(int argc, char **argv) {
    trinary_v1_init();
    int i = 1;
    const char *cmd = NULL;

    if (argc < 2) { print_help(); return 0; }
    while (i < argc && apply_opt_flag(argv[i])) i++;
    if (i >= argc) { print_help(); return 0; }
    cmd = argv[i];

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help(); return 0;
    }
    if (strcmp(cmd, "--version") == 0) {
        printf("%s\n", trinary_v1_version()); return 0;
    }
    if (strcmp(cmd, "--features") == 0) {
        uint32_t f = trinary_v1_cpu_features();
        printf("sse2=%d sse42=%d avx=%d avx2=%d avx512f=%d bmi2=%d popcnt=%d fma=%d\n",
               !!(f & TRINARY_CPU_SSE2),  !!(f & TRINARY_CPU_SSE42),
               !!(f & TRINARY_CPU_AVX),   !!(f & TRINARY_CPU_AVX2),
               !!(f & TRINARY_CPU_AVX512F), !!(f & TRINARY_CPU_BMI2),
               !!(f & TRINARY_CPU_POPCNT), !!(f & TRINARY_CPU_FMA));
        printf("braincore=%s harding=%s flip=%s rotor=%s prime=%s\n",
               trinary_v1_active_variant("braincore"),
               trinary_v1_active_variant("harding"),
               trinary_v1_active_variant("flip"),
               trinary_v1_active_variant("rotor"),
               trinary_v1_active_variant("prime"));
        return 0;
    }
    if (strcmp(cmd, "braincore") == 0) {
        long long n = 0, k = 0;
        if (i + 1 < argc) parse_int(argv[i + 1], &n);
        if (i + 2 < argc) parse_int(argv[i + 2], &k);
        return cmd_braincore(n, k);
    }
    if (strcmp(cmd, "bench") == 0) {
        return cmd_bench();
    }
    if (strcmp(cmd, "prime") == 0) {
        long long limit = 1000000;
        if (i + 1 < argc) parse_int(argv[i + 1], &limit);
        return cmd_prime(limit);
    }
    /* else: treat as .tri file */
    trinary_v1_status rc = trinary_v1_run_file(cmd);
    return rc == TRINARY_OK ? 0 : 2;
}
