/*
 * braincore_scalar.c — portable C fallback for the 8-bit LIF kernel.
 * Compiles on any C99+ target; correctness reference for the AVX2 asm.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#include <stdint.h>

void trinary_v1_braincore_u8_scalar(
    uint8_t *potentials,
    const uint8_t *inputs,
    size_t count,
    size_t iterations,
    uint8_t threshold)
{
    if (!potentials || !inputs || count == 0 || iterations == 0) return;
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t i = 0; i < count; ++i) {
            unsigned s = (unsigned)potentials[i] + (unsigned)inputs[i];
            if (s > 255) s = 255;
            if ((uint8_t)s >= threshold) s = 0;
            potentials[i] = (uint8_t)s;
        }
    }
}
