/*
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#ifndef TRINARY_TRI_IR_H_
#define TRINARY_TRI_IR_H_

#include <stdint.h>
#include <stddef.h>

typedef enum tri_ir_kind {
    TRI_IR_NOP = 0,
    TRI_IR_BRAINCORE = 1,
    TRI_IR_PRIME = 2
} tri_ir_kind;

typedef struct tri_ir_braincore {
    long long neurons;
    long long iterations;
} tri_ir_braincore;

typedef struct tri_ir_prime {
    long long limit;
} tri_ir_prime;

typedef struct tri_ir_inst {
    tri_ir_kind kind;
    union {
        tri_ir_braincore braincore;
        tri_ir_prime prime;
    } as;
} tri_ir_inst;

typedef struct tri_ir_program {
    tri_ir_inst *insts;
    size_t count;
    size_t capacity;
} tri_ir_program;

int tri_ir_execute(const tri_ir_inst *inst);
void tri_ir_program_init(tri_ir_program *program);
void tri_ir_program_free(tri_ir_program *program);
int tri_ir_program_push(tri_ir_program *program, tri_ir_inst inst);
int tri_ir_program_execute(const tri_ir_program *program);

#endif
