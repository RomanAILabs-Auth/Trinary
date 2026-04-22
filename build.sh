#!/usr/bin/env bash
# Trinary + TriPy POSIX build orchestrator.
#
# Mirrors build.ps1. Uses the system `cc` (clang or gcc) by default; override
# with CC=clang or CC="zig cc" for reproducibility.
#
#   ./build.sh            # Release build
#   ./build.sh Debug      # Debug build
#   ./build.sh Release 1  # Release + run tests

set -euo pipefail

CONFIG="${1:-Release}"
RUN_TESTS="${2:-0}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$ROOT/build"
OBJ="$BUILD/obj"
LIB="$BUILD/lib"
BIN="$BUILD/bin"

CC="${CC:-cc}"
AR="${AR:-ar}"

mkdir -p "$OBJ" "$LIB" "$BIN"

INCLUDE="-I$ROOT/engine/include"
STD="-std=c11"
WARN="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-function"
ARCH="-mavx2 -mxsave -mxsaveopt -mpopcnt -mbmi2 -mfma"

if [ "$CONFIG" = "Debug" ]; then
  OPT="-O0 -g -DDEBUG $ARCH"
  VARIANT="debug"
else
  OPT="-O3 -DNDEBUG -ffunction-sections -fdata-sections $ARCH"
  VARIANT="release"
fi
DEFS="-DTRINARY_BUILD_VARIANT=$VARIANT -D_POSIX_C_SOURCE=200809L"

ASM_SOURCES=(
  engine/asm/x86_64/braincore_u8_avx2.S
  engine/asm/x86_64/harding_gate_i16_avx2.S
  engine/asm/x86_64/lattice_flip_avx2.S
)

C_SOURCES=(
  engine/src/cpuid.c
  engine/src/allocator.c
  engine/src/dispatch.c
  engine/src/capi.c
  engine/src/rng.c
  engine/src/timing.c
  engine/src/version.c
  engine/src/braincore_scalar.c
  engine/src/harding_gate_scalar.c
  engine/src/lattice_flip_scalar.c
  engine/src/rotor_cl4_avx2.c
  engine/src/rotor_cl4_scalar.c
  engine/src/prime_sieve_scalar.c
  engine/compiler/lexer.c
  engine/compiler/sema.c
  engine/compiler/ir.c
  engine/compiler/optimizer.c
  engine/compiler/parser.c
)

OBJS=()

echo "[build] assembling AVX2 kernels ..."
for s in "${ASM_SOURCES[@]}"; do
  out="$OBJ/$(basename "$s" .S).o"
  $CC -c "$ROOT/$s" -o "$out" $INCLUDE $OPT
  OBJS+=("$out")
done

echo "[build] compiling C sources ..."
for s in "${C_SOURCES[@]}"; do
  out="$OBJ/$(basename "$s" .c).o"
  $CC -c "$ROOT/$s" -o "$out" $STD $INCLUDE $OPT $WARN $DEFS
  OBJS+=("$out")
done

echo "[build] creating libtrinary.a ..."
LIB_PATH="$LIB/libtrinary.a"
rm -f "$LIB_PATH"
$AR rcs "$LIB_PATH" "${OBJS[@]}"

echo "[build] linking trinary ..."
EXE_PATH="$BIN/trinary"
$CC "$ROOT/engine/src/main.c" -o "$EXE_PATH" "$LIB_PATH" $STD $INCLUDE $OPT $WARN $DEFS -lm

echo "[build] linking trinary-bench ..."
BENCH_PATH="$BIN/trinary-bench"
$CC "$ROOT/benchmarks/bench_engine.c" -o "$BENCH_PATH" "$LIB_PATH" $STD $INCLUDE $OPT $WARN $DEFS -lm

echo "[build] OK"
echo "  exe: $EXE_PATH"
echo "  lib: $LIB_PATH"

if [ "$RUN_TESTS" = "1" ]; then
  echo "[test] compiling engine tests ..."
  T_OUT="$BIN/trinary_tests"
  $CC "$ROOT/engine/tests/test_kernels.c" -o "$T_OUT" "$LIB_PATH" $STD $INCLUDE "-I$ROOT/engine/compiler" $OPT $WARN $DEFS -lm
  "$T_OUT"
fi
