/*
 * version.c — Build metadata.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 */
#include "trinary/trinary.h"

#ifndef TRINARY_BUILD_VARIANT
#define TRINARY_BUILD_VARIANT release
#endif

#define TRI_STR_(x) #x
#define TRI_STR(x) TRI_STR_(x)

const char *trinary_v1_version(void) {
    return "trinary " TRINARY_VERSION_STRING " (clang, x86_64, "
           TRI_STR(TRINARY_BUILD_VARIANT) ")";
}
