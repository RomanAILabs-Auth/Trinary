/*
 * lattice_flip_scalar.c — portable reference for lattice flip.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#include <stdint.h>

void trinary_v1_lattice_flip_scalar(
    uint64_t *lattice,
    size_t word_count,
    size_t iterations,
    uint64_t mask)
{
    if (!lattice || word_count == 0 || iterations == 0) return;
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t i = 0; i < word_count; ++i) {
            lattice[i] ^= mask;
        }
    }
}
