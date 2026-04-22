/*
 * ir.c — v0 executable IR layer for .tri kernel statements.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * This layer keeps parser logic focused on syntax while runtime execution for
 * kernel operations lives behind explicit IR op structs.
 */
#include "ir.h"

#include "trinary/trinary.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static int exec_braincore(long long neurons, long long iterations) {
    uint8_t *pot = (uint8_t *)trinary_v1_alloc((size_t)neurons, 64);
    uint8_t *inp = (uint8_t *)trinary_v1_alloc((size_t)neurons, 64);
    if (!pot || !inp) {
        if (pot) trinary_v1_free(pot);
        if (inp) trinary_v1_free(inp);
        fprintf(stderr, "[tri] alloc failed\n");
        return -1;
    }

    trinary_v1_rng_seed(0);
    trinary_v1_rng_fill(inp, (size_t)neurons);
    for (long long i = 0; i < neurons; ++i) inp[i] %= 50;
    for (long long i = 0; i < neurons; ++i) pot[i] = 0;

    double t0 = trinary_v1_now_seconds();
    trinary_v1_status rc = trinary_v1_braincore_u8(
        pot, inp, (size_t)neurons, (size_t)iterations, 200);
    double t1 = trinary_v1_now_seconds();

    trinary_v1_free(pot);
    trinary_v1_free(inp);
    if (rc != TRINARY_OK) return -1;

    const double elapsed = t1 - t0;
    const double ops = (double)neurons * (double)iterations;
    printf("[trinary] braincore: %.4f s  (%.2f GNeurons/s, variant=%s)\n",
           elapsed, ops / elapsed / 1e9, trinary_v1_active_variant("braincore"));
    fflush(stdout);
    return 0;
}

static int exec_prime(long long limit) {
    uint64_t count = 0;
    double t0 = trinary_v1_now_seconds();
    trinary_v1_status rc = trinary_v1_prime_sieve_u64((uint64_t)limit, &count);
    double t1 = trinary_v1_now_seconds();
    if (rc != TRINARY_OK) return -1;

    const double elapsed = t1 - t0;
    printf("[trinary] prime: limit=%lld count=%llu time=%.6f s  (%.2f Mnums/s, variant=%s)\n",
           limit,
           (unsigned long long)count,
           elapsed,
           elapsed > 0.0 ? ((double)limit / elapsed / 1e6) : 0.0,
           trinary_v1_active_variant("prime"));
    fflush(stdout);
    return 0;
}

int tri_ir_execute(const tri_ir_inst *inst) {
    if (!inst) return -1;
    switch (inst->kind) {
        case TRI_IR_NOP:
            return 0;
        case TRI_IR_BRAINCORE:
            return exec_braincore(inst->as.braincore.neurons, inst->as.braincore.iterations);
        case TRI_IR_PRIME:
            return exec_prime(inst->as.prime.limit);
        default:
            fprintf(stderr, "[tri] ir error: unknown opcode %d\n", (int)inst->kind);
            return -1;
    }
}

void tri_ir_program_init(tri_ir_program *program) {
    if (!program) return;
    program->insts = NULL;
    program->count = 0;
    program->capacity = 0;
}

void tri_ir_program_free(tri_ir_program *program) {
    if (!program) return;
    free(program->insts);
    program->insts = NULL;
    program->count = 0;
    program->capacity = 0;
}

int tri_ir_program_push(tri_ir_program *program, tri_ir_inst inst) {
    tri_ir_inst *grown = NULL;
    size_t new_cap = 0;
    if (!program) return -1;

    if (program->count == program->capacity) {
        new_cap = program->capacity == 0 ? 8 : program->capacity * 2;
        grown = (tri_ir_inst *)realloc(program->insts, new_cap * sizeof(*grown));
        if (!grown) return -1;
        program->insts = grown;
        program->capacity = new_cap;
    }

    program->insts[program->count++] = inst;
    return 0;
}

int tri_ir_program_execute(const tri_ir_program *program) {
    if (!program) return -1;
    for (size_t i = 0; i < program->count; ++i) {
        if (tri_ir_execute(&program->insts[i]) != 0) return -1;
    }
    return 0;
}
