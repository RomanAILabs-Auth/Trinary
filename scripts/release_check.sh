#!/usr/bin/env bash
# Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
# Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
# Contact: daniel@romanailabs.com, romanailabs@gmail.com
# Website: romanailabs.com
set -euo pipefail

RUN_TESTS="${1:-0}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

assert_file() {
  local p="$1"
  [[ -f "$p" ]] || { echo "missing required file: $p" >&2; exit 1; }
}

assert_contains() {
  local p="$1"
  local needle="$2"
  rg -F --quiet -- "$needle" "$p" || {
    echo "required text not found in $p : $needle" >&2
    exit 1
  }
}

echo "[release-check] verifying required files..."
assert_file "$ROOT/LICENSE"
assert_file "$ROOT/NOTICE"
assert_file "$ROOT/README.md"
assert_file "$ROOT/engine/README.md"
assert_file "$ROOT/tripy/README.md"
assert_file "$ROOT/scripts/install_trinary.ps1"
assert_file "$ROOT/scripts/install_tripy.ps1"
assert_file "$ROOT/scripts/install_trinary.sh"
assert_file "$ROOT/scripts/install_tripy.sh"
assert_file "$ROOT/trinary-tripy-architecture.md"

echo "[release-check] verifying licensing/attribution markers..."
assert_contains "$ROOT/LICENSE" "RomanAILabs Business Source License 1.1"
assert_contains "$ROOT/NOTICE" "ChatGPT-5.4 / OpenAI"
assert_contains "$ROOT/README.md" "ChatGPT-5.4 / OpenAI"
assert_contains "$ROOT/tripy/README.md" "ChatGPT-5.4 / OpenAI"
assert_contains "$ROOT/engine/README.md" "ChatGPT-5.4 / OpenAI"

echo "[release-check] verifying architecture/license consistency..."
assert_contains "$ROOT/trinary-tripy-architecture.md" "RomanAILabs Business Source License 1.1"

if [[ "$RUN_TESTS" == "1" ]]; then
  echo "[release-check] running full build + tests..."
  "$ROOT/build.sh" Release 1
fi

echo "[release-check] OK"
