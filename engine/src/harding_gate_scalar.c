/*
 * harding_gate_scalar.c — portable reference for Harding Gate over int16.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#include <stdint.h>

void trinary_v1_harding_gate_i16_scalar(
    int16_t *out,
    const int16_t *a,
    const int16_t *b,
    size_t count)
{
    if (!out || !a || !b) return;
    for (size_t i = 0; i < count; ++i) {
        int32_t prod = (int32_t)a[i] * (int32_t)b[i];
        int32_t xored = (int32_t)((uint16_t)a[i] ^ (uint16_t)b[i]);
        out[i] = (int16_t)((prod & 0xFFFF) - (xored & 0xFFFF));
    }
}
