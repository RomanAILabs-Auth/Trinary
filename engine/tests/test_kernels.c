/*
 * test_kernels.c — correctness tests for the machine-code kernels.
 * Verifies each AVX2 kernel produces the same output as the scalar reference.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"
#include "ir.h"
#include "optimizer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern void trinary_v1_braincore_u8_scalar(uint8_t *, const uint8_t *, size_t, size_t, uint8_t);
extern void trinary_v1_harding_gate_i16_scalar(int16_t *, const int16_t *, const int16_t *, size_t);
extern void trinary_v1_lattice_flip_scalar(uint64_t *, size_t, size_t, uint64_t);
extern void trinary_v1_rotor_cl4_f32_scalar(float *, const float *, size_t, float, float);
extern uint64_t trinary_v1_prime_sieve_u64_scalar(uint64_t);

static int failures = 0;

static void check(const char *name, int ok) {
    printf("  %-40s %s\n", name, ok ? "ok" : "FAIL");
    if (!ok) failures++;
}

static void test_braincore(void) {
    const size_t N = 1024;
    const size_t I = 17;
    uint8_t *pot_a = trinary_v1_alloc(N, 32);
    uint8_t *pot_b = trinary_v1_alloc(N, 32);
    uint8_t *inp   = trinary_v1_alloc(N, 32);
    trinary_v1_rng_seed(0xC0FFEE);
    trinary_v1_rng_fill(inp, N);
    for (size_t i = 0; i < N; ++i) inp[i] %= 40;
    memset(pot_a, 0, N);
    memset(pot_b, 0, N);
    trinary_v1_braincore_u8_scalar(pot_a, inp, N, I, 200);
    trinary_v1_status rc = trinary_v1_braincore_u8(pot_b, inp, N, I, 200);
    check("braincore: status OK",       rc == TRINARY_OK);
    check("braincore: matches scalar",  memcmp(pot_a, pot_b, N) == 0);
    trinary_v1_free(pot_a); trinary_v1_free(pot_b); trinary_v1_free(inp);
}

static void test_harding(void) {
    const size_t N = 512;
    int16_t *a = trinary_v1_alloc(N * sizeof(int16_t), 32);
    int16_t *b = trinary_v1_alloc(N * sizeof(int16_t), 32);
    int16_t *o1 = trinary_v1_alloc(N * sizeof(int16_t), 32);
    int16_t *o2 = trinary_v1_alloc(N * sizeof(int16_t), 32);
    trinary_v1_rng_seed(42);
    trinary_v1_rng_fill(a, N * sizeof(int16_t));
    trinary_v1_rng_fill(b, N * sizeof(int16_t));
    trinary_v1_harding_gate_i16_scalar(o1, a, b, N);
    trinary_v1_status rc = trinary_v1_harding_gate_i16(o2, a, b, N);
    check("harding:   status OK",      rc == TRINARY_OK);
    check("harding:   matches scalar", memcmp(o1, o2, N * sizeof(int16_t)) == 0);
    trinary_v1_free(a); trinary_v1_free(b); trinary_v1_free(o1); trinary_v1_free(o2);
}

static void test_flip(void) {
    const size_t W = 256;
    uint64_t *la = trinary_v1_alloc(W * sizeof(uint64_t), 32);
    uint64_t *lb = trinary_v1_alloc(W * sizeof(uint64_t), 32);
    trinary_v1_rng_seed(7);
    trinary_v1_rng_fill(la, W * sizeof(uint64_t));
    memcpy(lb, la, W * sizeof(uint64_t));
    trinary_v1_lattice_flip_scalar(la, W, 5, 0xDEADBEEFCAFEBABEULL);
    trinary_v1_status rc = trinary_v1_lattice_flip(lb, W, 5, 0xDEADBEEFCAFEBABEULL);
    check("flip:      status OK",      rc == TRINARY_OK);
    check("flip:      matches scalar", memcmp(la, lb, W * sizeof(uint64_t)) == 0);
    trinary_v1_free(la); trinary_v1_free(lb);
}

static void test_rotor(void) {
    const size_t N = 128;
    float *in = trinary_v1_alloc(N * 4 * sizeof(float), 32);
    float *a  = trinary_v1_alloc(N * 4 * sizeof(float), 32);
    float *b  = trinary_v1_alloc(N * 4 * sizeof(float), 32);
    trinary_v1_rng_seed(17);
    trinary_v1_rng_fill(in, N * 4 * sizeof(float));
    for (size_t i = 0; i < N * 4; ++i) in[i] = (in[i] - 128.0f) * 0.01f;
    trinary_v1_rotor_cl4_f32_scalar(a, in, N, 0.37f, -0.23f);
    trinary_v1_status rc = trinary_v1_rotor_cl4_f32(b, in, N, 0.37f, -0.23f);
    check("rotor:     status OK", rc == TRINARY_OK);
    int same = 1;
    for (size_t i = 0; i < N * 4; ++i) {
        if (fabsf(a[i] - b[i]) > 1e-6f) { same = 0; break; }
    }
    check("rotor:     matches scalar", same);
    trinary_v1_free(in); trinary_v1_free(a); trinary_v1_free(b);
}

static void test_prime(void) {
    uint64_t out = 0;
    trinary_v1_status rc = trinary_v1_prime_sieve_u64(100, &out);
    check("prime:     status OK", rc == TRINARY_OK);
    check("prime:     pi(100)==25", out == 25);
    uint64_t ref = trinary_v1_prime_sieve_u64_scalar(100000);
    rc = trinary_v1_prime_sieve_u64(100000, &out);
    check("prime:     pi(1e5) matches ref", rc == TRINARY_OK && out == ref);
}

static void test_allocator(void) {
    void *p1 = trinary_v1_alloc(1, 64);
    void *p2 = trinary_v1_alloc(1024, 32); /* API should round up to 64 internally */
    int ok1 = (p1 != NULL) && ((((uintptr_t)p1) & 63u) == 0);
    int ok2 = (p2 != NULL) && ((((uintptr_t)p2) & 63u) == 0);
    check("alloc:     64-byte aligned (explicit)", ok1);
    check("alloc:     64-byte aligned (rounded)", ok2);
    trinary_v1_free(p1);
    trinary_v1_free(p2);
}

static void test_ir_optimizer(void) {
    tri_ir_inst inst = {
        .kind = TRI_IR_BRAINCORE,
        .as.braincore = {.neurons = 33, .iterations = 0}
    };
    int rc = tri_optimize_ir_inst(&inst);
    check("opt:       optimize inst OK", rc == 0);
    check("opt:       neurons rounded", inst.as.braincore.neurons == 64);
    check("opt:       iters defaulted", inst.as.braincore.iterations == 1000);
}

static void test_ir_program_pipeline(void) {
    tri_ir_program p;
    tri_ir_program_init(&p);

    tri_ir_inst inst = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };

    int rc_push = tri_ir_program_push(&p, inst);
    int rc_opt = tri_optimize_ir_program(&p);
    int rc_exec = tri_ir_program_execute(&p);

    check("ir:        program push OK", rc_push == 0);
    check("ir:        program optimize OK", rc_opt == 0);
    check("ir:        program execute OK", rc_exec == 0);
    tri_ir_program_free(&p);
}

static void test_ir_program_canonicalization(void) {
    tri_ir_program p;
    tri_ir_program_init(&p);

    tri_ir_inst a = {
        .kind = TRI_IR_BRAINCORE,
        .as.braincore = {.neurons = 33, .iterations = 0}
    };
    tri_ir_inst b = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = -7}
    };

    int rc_a = tri_ir_program_push(&p, a);
    int rc_b = tri_ir_program_push(&p, b);
    tri_opt_report report;
    int rc_opt = tri_optimize_ir_program_report(&p, &report);

    check("ir:        push braincore", rc_a == 0);
    check("ir:        push prime", rc_b == 0);
    check("ir:        optimize program OK", rc_opt == 0);
    check("ir:        prog braincore neurons rounded",
          p.count >= 1 && p.insts[0].as.braincore.neurons == 64);
    check("ir:        prog braincore iters defaulted",
          p.count >= 1 && p.insts[0].as.braincore.iterations == 1000);
    check("ir:        prog prime limit clamped",
          p.count >= 2 && p.insts[1].as.prime.limit == 0);
    check("ir:        report seen count", report.seen_insts == 2);
    check("ir:        report canonicalized count", report.canonicalized_insts == 2);
    check("ir:        report transformed count", report.transformed_insts == 2);
    check("ir:        report pass count", report.pass_runs == 2);
    check("ir:        report redundancy candidates", report.redundancy_candidates == 0);
    tri_ir_program_free(&p);
}

static void test_ir_redundancy_scan(void) {
    tri_ir_program p;
    tri_ir_program_init(&p);

    tri_ir_inst a = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };
    tri_ir_inst b = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };
    tri_opt_report report;

    check("ir:        push prime a", tri_ir_program_push(&p, a) == 0);
    check("ir:        push prime b", tri_ir_program_push(&p, b) == 0);
    check("ir:        optimize+scan OK",
          tri_optimize_ir_program_with_config(&p, NULL, &report) == 0);
    check("ir:        scan sees candidate", report.redundancy_candidates == 1);
    check("ir:        no transform by default", p.count >= 2 && p.insts[1].kind == TRI_IR_PRIME);
    tri_ir_program_free(&p);
}

static void test_ir_experimental_transform_opt_in(void) {
    tri_ir_program p;
    tri_ir_program_init(&p);

    tri_ir_inst a = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };
    tri_ir_inst b = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };
    tri_opt_report report;
    tri_opt_config config = {
        .enable_experimental_transforms = 1,
        .allow_observable_output_changes = 1
    };

    check("ir:        push exp prime a", tri_ir_program_push(&p, a) == 0);
    check("ir:        push exp prime b", tri_ir_program_push(&p, b) == 0);
    check("ir:        optimize+exp OK",
          tri_optimize_ir_program_with_config(&p, &config, &report) == 0);
    check("ir:        exp candidate count", report.redundancy_candidates == 1);
    check("ir:        exp transformed", p.count >= 2 && p.insts[1].kind == TRI_IR_NOP);
    tri_ir_program_free(&p);
}

static void test_ir_observable_without_experimental_no_transform(void) {
    tri_ir_program p;
    tri_ir_program_init(&p);

    tri_ir_inst a = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };
    tri_ir_inst b = {
        .kind = TRI_IR_PRIME,
        .as.prime = {.limit = 100}
    };
    tri_opt_report report;
    tri_opt_config config = {
        .enable_experimental_transforms = 0,
        .allow_observable_output_changes = 1
    };

    check("ir:        push obs prime a", tri_ir_program_push(&p, a) == 0);
    check("ir:        push obs prime b", tri_ir_program_push(&p, b) == 0);
    check("ir:        optimize+obs-only OK",
          tri_optimize_ir_program_with_config(&p, &config, &report) == 0);
    check("ir:        obs-only candidate count", report.redundancy_candidates == 1);
    check("ir:        obs-only no transform", p.count >= 2 && p.insts[1].kind == TRI_IR_PRIME);
    tri_ir_program_free(&p);
}

static void test_argchecks(void) {
    uint8_t buf[32] = {0};
    /* misaligned */
    check("braincore: rejects unaligned",
          trinary_v1_braincore_u8(buf + 1, buf, 32, 1, 200) == TRINARY_ERR_ARGS);
    check("harding:   rejects nulls",
          trinary_v1_harding_gate_i16(NULL, NULL, NULL, 16) == TRINARY_ERR_ARGS);
    check("flip:      rejects bad count",
          trinary_v1_lattice_flip((uint64_t*)buf, 3, 1, 0xFFFF) == TRINARY_ERR_ARGS);
    check("rotor:     rejects nulls",
          trinary_v1_rotor_cl4_f32(NULL, NULL, 1, 0.1f, 0.2f) == TRINARY_ERR_ARGS);
    check("prime:     rejects null out",
          trinary_v1_prime_sieve_u64(100, NULL) == TRINARY_ERR_ARGS);
}

int main(void) {
    trinary_v1_init();
    printf("trinary engine test suite\n");
    printf("version: %s\n", trinary_v1_version());
    printf("variants: braincore=%s harding=%s flip=%s rotor=%s prime=%s\n",
           trinary_v1_active_variant("braincore"),
           trinary_v1_active_variant("harding"),
           trinary_v1_active_variant("flip"),
           trinary_v1_active_variant("rotor"),
           trinary_v1_active_variant("prime"));
    test_braincore();
    test_harding();
    test_flip();
    test_rotor();
    test_prime();
    test_allocator();
    test_ir_optimizer();
    test_ir_program_pipeline();
    test_ir_program_canonicalization();
    test_ir_redundancy_scan();
    test_ir_experimental_transform_opt_in();
    test_ir_observable_without_experimental_no_transform();
    test_argchecks();
    printf("%s\n", failures == 0 ? "ALL PASS" : "FAILURES");
    return failures ? 1 : 0;
}
