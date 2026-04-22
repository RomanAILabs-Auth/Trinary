/*
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#ifndef TRINARY_TRI_OPTIMIZER_H_
#define TRINARY_TRI_OPTIMIZER_H_

#include "ir.h"

/*
 * v0 optimizer stage:
 * - canonicalizes and validates IR operands before execution
 * - keeps behavior byte-for-byte equivalent to current interpreter semantics
 */
typedef struct tri_opt_report {
    size_t seen_insts;
    size_t canonicalized_insts;
    size_t transformed_insts;
    size_t pass_runs;
    size_t redundancy_candidates;
} tri_opt_report;

typedef struct tri_opt_config {
    int enable_experimental_transforms;
    int allow_observable_output_changes;
} tri_opt_config;

int tri_optimize_ir_inst(tri_ir_inst *inst);
int tri_optimize_ir_program(tri_ir_program *program);
int tri_optimize_ir_program_report(tri_ir_program *program, tri_opt_report *report);
int tri_optimize_ir_program_with_config(
    tri_ir_program *program, const tri_opt_config *config, tri_opt_report *report);

#endif
