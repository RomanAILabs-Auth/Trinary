#!/usr/bin/env bash
# Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
# Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
# Contact: daniel@romanailabs.com, romanailabs@gmail.com
# Website: romanailabs.com
set -euo pipefail

CONFIG="${1:-Release}"
SKIP_BUILD="${SKIP_BUILD:-0}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${HOME}/.local/bin"
SRC="${ROOT}/build/bin/trinary"
DST="${BIN_DIR}/trinary"

if [[ "${SKIP_BUILD}" != "1" ]]; then
  "${ROOT}/build.sh" "${CONFIG}"
fi

if [[ ! -f "${SRC}" ]]; then
  echo "missing binary: ${SRC}" >&2
  exit 1
fi

mkdir -p "${BIN_DIR}"
cp "${SRC}" "${DST}"
chmod +x "${DST}"

echo "[install] trinary installed to: ${DST}"
echo "[install] ensure ${BIN_DIR} is in PATH, then run: trinary --version"
