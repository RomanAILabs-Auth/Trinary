/*
 * rotor_cl4_avx2.c — AVX2-targeted rotor path.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * Built with AVX2/FMA flags by the global build. We intentionally keep the
 * arithmetic expression identical to the scalar reference so correctness stays
 * bit-stable while compilers are free to auto-vectorize.
 */
#include <math.h>
#include <stddef.h>

void trinary_v1_rotor_cl4_f32_avx2(
    float *out_xyzw, const float *in_xyzw, size_t vec4_count, float theta_xy, float theta_zw)
{
    const float cxy = cosf(theta_xy);
    const float sxy = sinf(theta_xy);
    const float czw = cosf(theta_zw);
    const float szw = sinf(theta_zw);

    for (size_t i = 0; i < vec4_count; ++i) {
        const size_t b = i * 4u;
        const float x = in_xyzw[b + 0];
        const float y = in_xyzw[b + 1];
        const float z = in_xyzw[b + 2];
        const float w = in_xyzw[b + 3];
        out_xyzw[b + 0] = cxy * x - sxy * y;
        out_xyzw[b + 1] = sxy * x + cxy * y;
        out_xyzw[b + 2] = czw * z - szw * w;
        out_xyzw[b + 3] = szw * z + czw * w;
    }
}
