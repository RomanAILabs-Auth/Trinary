# engine/asm/aarch64/

**Status:** reserved. Phase 1+ — **no AArch64 kernels ship in v0.1.**

## Scope when opened

One hand-written NEON (ARMv8-A) source per shipped kernel, plus a SVE2 variant
per kernel where throughput gain > 10 % and an ARMv9 machine is available in CI:

- `braincore_u8_neon.S`        / `braincore_u8_sve2.S`
- `harding_gate_i16_neon.S`    / `harding_gate_i16_sve2.S`
- `lattice_flip_neon.S`        / `lattice_flip_sve2.S`
- `rotor_cl4_neon.S`           / `rotor_cl4_sve2.S`
- `prime_sieve_neon.S`         / `prime_sieve_sve2.S`
- `entry_sysv_aarch64.S`       (libc-free entry; see §3 of architecture doc)

## Contract

1. Every NEON kernel is byte-exact against its scalar reference
   (same test harness as x86-64 in `engine/tests/test_kernels.c`).
2. SVE2 kernels are gated behind runtime detection (`ID_AA64PFR0_EL1.SVE`
   and `ID_AA64ZFR0_EL1.SVEver`), never selected blindly.
3. Dispatch table (`engine/src/dispatch.c`) gains AArch64 rows that mirror
   the x86-64 layout.
4. CI gains an `aarch64-build-and-test` job (Linux arm64 runners, macOS on
   Apple Silicon) before this directory gets any non-README content.

## Why the directory exists now if it's empty

§2 of the architecture doc mandates this path. Creating the empty directory
with this README makes the compliance matrix (§2.1) green on structure
while keeping the deferral honest.
