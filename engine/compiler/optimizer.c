/*
 * optimizer.c — v0 IR canonicalization pass.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "optimizer.h"

#include "sema.h"

static int canonicalize_inst(tri_ir_inst *inst, int *changed) {
    if (!inst || !changed) return -1;
    *changed = 0;

    switch (inst->kind) {
        case TRI_IR_BRAINCORE: {
            const long long before_neurons = inst->as.braincore.neurons;
            const long long before_iters = inst->as.braincore.iterations;
            tri_sema_finalize_braincore(
                &inst->as.braincore.neurons, &inst->as.braincore.iterations);
            *changed = (inst->as.braincore.neurons != before_neurons) ||
                       (inst->as.braincore.iterations != before_iters);
            return 0;
        }
        case TRI_IR_PRIME: {
            const long long before_limit = inst->as.prime.limit;
            tri_sema_finalize_prime(&inst->as.prime.limit);
            *changed = inst->as.prime.limit != before_limit;
            return 0;
        }
        default:
            return -1;
    }
}

int tri_optimize_ir_inst(tri_ir_inst *inst) {
    int changed = 0;
    return canonicalize_inst(inst, &changed);
}

int tri_optimize_ir_program(tri_ir_program *program) {
    return tri_optimize_ir_program_with_config(program, NULL, NULL);
}

int tri_optimize_ir_program_report(tri_ir_program *program, tri_opt_report *report) {
    return tri_optimize_ir_program_with_config(program, NULL, report);
}

int tri_optimize_ir_program_with_config(
    tri_ir_program *program, const tri_opt_config *config, tri_opt_report *report) {
    if (!program) return -1;
    if (report) {
        report->seen_insts = 0;
        report->canonicalized_insts = 0;
        report->transformed_insts = 0;
        report->pass_runs = 0;
        report->redundancy_candidates = 0;
    }

    if (report) report->pass_runs++;
    for (size_t i = 0; i < program->count; ++i) {
        int changed = 0;
        if (canonicalize_inst(&program->insts[i], &changed) != 0) return -1;
        if (report) {
            report->seen_insts++;
            report->canonicalized_insts++;
            if (changed) report->transformed_insts++;
        }
    }

    if (report) report->pass_runs++;
    for (size_t i = 1; i < program->count; ++i) {
        tri_ir_inst *a = &program->insts[i - 1];
        tri_ir_inst *b = &program->insts[i];
        if (a->kind == TRI_IR_PRIME && b->kind == TRI_IR_PRIME &&
            a->as.prime.limit == b->as.prime.limit) {
            if (report) report->redundancy_candidates++;
            /*
             * Intentional no-op for now: prime ops emit visible timing output, so
             * removing/fusing them would change externally observed behavior.
             */
            if (config &&
                config->enable_experimental_transforms &&
                config->allow_observable_output_changes) {
                b->kind = TRI_IR_NOP;
                if (report) report->transformed_insts++;
            }
        }
    }
    return 0;
}
