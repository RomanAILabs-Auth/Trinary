#!/usr/bin/env bash
# Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
# Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
# Contact: daniel@romanailabs.com, romanailabs@gmail.com
# Website: romanailabs.com
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TRIPY_ROOT="${ROOT}/tripy"

python -m pip install -e "${TRIPY_ROOT}"
echo "[install] tripy installed in editable mode from: ${TRIPY_ROOT}"
echo "[install] verify with: tripy --version"
