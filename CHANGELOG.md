# Changelog

All notable changes to this project will be documented in this file.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning: [SemVer](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added — licensing/docs/install hardening
- Replaced repository license with RomanAILabs Business Source License 1.1
  (`LICENSE`) as provided by project owner.
- Added `NOTICE` with collaborator recognition and author contact information.
- Rewrote root README as dual-repo operator/developer guide for Trinary + TriPy.
- Added dedicated `engine/README.md` and expanded `tripy/README.md` with
  Human/LLM coding guidance and runtime control documentation.
- Added installer scripts for both components on Windows and POSIX:
  - `scripts/install_trinary.ps1`, `scripts/install_trinary.sh`
  - `scripts/install_tripy.ps1`, `scripts/install_tripy.sh`
- Updated source file attribution headers in primary front-door files:
  - `engine/src/main.c`
  - `engine/compiler/parser.c`
  - `tripy/src/tripy/cli.py`
  - `tripy/src/tripy/__init__.py`

### Changed
- `tripy/pyproject.toml` project license text now references RBSL 1.1 and no
  longer declares Apache classifier metadata.
- Applied repository-wide attribution/contact header sweep across active engine,
  compiler, ASM, tests, and TriPy front-door source files.
- Added ChatGPT-5.4/OpenAI to collaborator attribution across docs, notice, and
  active source header blocks.
- Added release-readiness validation scripts:
  - `scripts/release_check.ps1`
  - `scripts/release_check.sh`
  which verify required files, licensing/attribution markers, and optionally run
  full build/tests.
- Added elite cross-language benchmark harness:
  - `benchmarks/elite_lang_bench.py`
  - outputs `benchmarks/elite_results.md` and `benchmarks/elite_results.json`
  - includes Trinary, TriPy, Python, Node.js, Go, C++, Rust, and Mojo lanes
    with toolchain availability reporting.
- Updated root and TriPy READMEs with benchmark snapshot and rerun instructions.

### Added — Prompt 2 execution
- New C ABI kernel endpoints in `trinary.h`:
  - `trinary_v1_rotor_cl4_f32` (reference 4D rotor transform over packed vec4<f32>).
  - `trinary_v1_prime_sieve_u64` (prime counting up to `limit`).
- Scalar implementations:
  - `engine/src/rotor_cl4_scalar.c`
  - `engine/src/prime_sieve_scalar.c`
- Dispatch table extended with `rotor` and `prime` slots/variants.
- Added AVX2-targeted rotor implementation `engine/src/rotor_cl4_avx2.c`.
- `trinary` CLI now supports `trinary prime [LIMIT]`.
- Engine test suite extended for rotor/prime correctness + argument validation.

### Changed
- `trinary --features` now reports active variants for `rotor` and `prime`.
- Build scripts (`build.ps1`, `build.sh`) now compile/link new scalar kernel sources.
- Dispatch now selects `rotor=avx2` on AVX2-capable hosts.
- Allocator internals now follow the §3.4 direction:
  - minimum alignment is 64 bytes (requested alignment is rounded up).
  - Windows path tries `VirtualAlloc(..., MEM_LARGE_PAGES)` then falls back.
  - POSIX path uses `mmap` with `MADV_HUGEPAGE` hint when available.
  - `trinary_v1_free` is metadata-driven and correctly unmaps/free()s origin.
- `prime_sieve` scalar path upgraded to a packed-bitset segmented implementation
  with popcount-based counting (higher throughput, lower memory traffic).
- Allocator strategy tuned for throughput: OS-mapped pages are now reserved for
  very large arenas (>=128 MiB), while normal kernel-sized buffers use fast heap
  backing with 64-byte aligned returns.
- Benchmark harness now includes `prime_sieve` as a first-class measured kernel.
- `benchmarks/baseline.json` now locks `prime_sieve` too; perf-gate enforces the
  same ±3% budget on prime throughput as on existing kernels.
- `.tri` front-end build now compiles split units:
  - `engine/compiler/lexer.c`
  - `engine/compiler/parser.c`
  replacing monolithic compilation of `engine/compiler/tri_lang.c`.
- `.tri` language now supports direct `prime(...)` kernel statements that route
  to `trinary_v1_prime_sieve_u64` from the parser/interpreter path.
- Lexer keyword classification hot path was tightened (length/first-char dispatch)
  to reduce per-token branching overhead.
- Added `engine/compiler/sema.c` + `sema.h` and wired parser checks through it:
  declaration extent validation, loop bound sanity checks, and kernel argument
  canonicalization for braincore/prime statements.
- Added `engine/compiler/ir.c` + `ir.h` as a v0 executable IR layer and moved
  `.tri` `braincore(...)` / `prime(...)` runtime execution behind typed IR ops.
- Added `engine/compiler/optimizer.c` + `optimizer.h` as a v0 optimizer pass and
  now canonicalize IR operands before execution (`parse -> IR -> optimize -> execute`).
- Added `tri_ir_program_*` container APIs in `engine/compiler/ir.c`/`ir.h` and
  `tri_optimize_ir_program(...)` for program-level optimization/execution flow.
- Parser kernel statements now run through the program pipeline (`IR program ->
  optimize -> execute`) while preserving existing behavior.
- Parser now batches consecutive kernel statements in a queued IR program and flushes
  on safe boundaries (non-kernel statement or EOF), enabling program-level optimizer
  scope while preserving statement order.
- Engine C tests now cover compiler-layer IR/optimizer invariants:
  - `tri_optimize_ir_inst` canonicalization (`braincore(33,0)` -> rounded/defaulted args)
  - `tri_ir_program_*` + `tri_optimize_ir_program` execution path sanity.
- Added TriPy smoke coverage for multi-op `.tri` execution path:
  - consecutive `prime(...)` statements execute and emit two kernel result lines.
- Added flush-boundary coverage for queued kernel IR execution:
  - `prime(...)`, then `print("marker")`, then `prime(...)` preserves order and
    proves kernel queue flushes before non-kernel statements.
- Expanded engine C coverage for program-level canonicalization:
  - multi-op IR program canonicalizes each instruction as expected.
- Added optimizer pass reporting scaffold:
  - `tri_opt_report` in `optimizer.h`
  - `tri_optimize_ir_program_report(...)` in `optimizer.c`
  - parser queue flush now routes through report-capable optimizer entrypoint.
- Added engine test assertions for optimizer reporting counters
  (`seen_insts`, `canonicalized_insts`, `transformed_insts`).
- Extended optimizer scaffold with pass-manager style config/report hooks:
  - `tri_opt_config`
  - `tri_optimize_ir_program_with_config(...)`
  - report counters for `pass_runs` and `redundancy_candidates`.
- Added conservative multi-op redundancy scan for consecutive identical `prime`
  ops (analysis-only, no transform yet to preserve observable output semantics).
- Added engine tests validating redundancy candidate detection and new report fields.
- Added `TRI_IR_NOP` opcode and guarded experimental transform path:
  - experimental duplicate-prime elimination now only applies when both
    `enable_experimental_transforms=1` and
    `allow_observable_output_changes=1` are explicitly set in config.
- Added engine tests for both behaviors:
  - default config keeps duplicate prime ops intact
  - explicit opt-in config converts redundant adjacent prime op to `TRI_IR_NOP`.
- Parser runtime now resolves optimizer config from env vars:
  - `TRINARY_OPT_EXPERIMENTAL`
  - `TRINARY_OPT_ALLOW_OBSERVABLE`
  so experimental transforms can be benchmarked without changing default behavior.
- Added optional optimizer telemetry trace env var:
  - `TRINARY_OPT_TRACE=1` emits per-queue optimizer report stats to stderr
    (`seen`, `canonicalized`, `transformed`, `passes`, `redundancy`).
- Optimizer trace path now emits both per-flush and run-total summary lines:
  - `[tri][opt] ...` per queued flush
  - `[tri][opt-total] ...` once per successful `.tri` run
  with parser-side aggregation to improve observability.
- Added first-class CLI optimizer toggles in `tripy`:
  - `--opt-default`
  - `--opt-experimental`
  - `--opt-allow-observable`
  - `--opt-trace`
  CLI now applies these as scoped runtime env overrides for the process.
- Added TriPy CLI smoke test proving env-opt-in path dedupes adjacent `prime`
  output while default tests still assert two outputs.
- Added TriPy CLI smoke test proving trace telemetry appears in stderr when enabled.
- Added TriPy CLI smoke tests for the new flag-based opt-in paths
  (`--opt-experimental --opt-allow-observable` and `--opt-trace`).
- Added additional fail-safe coverage for optimizer controls:
  - `--opt-allow-observable` alone does not enable transforms.
  - `--opt-default` overrides inherited optimizer env toggles.
  - engine C test confirms observable-only config does not transform without
    experimental mode enabled.
- Native `trinary` CLI now supports optimizer flags mirroring `tripy`:
  - `--opt-default`
  - `--opt-experimental`
  - `--opt-allow-observable`
  - `--opt-trace`
  and applies them before command/file dispatch.
- Added smoke tests covering native CLI optimizer flag behavior
  (trace output and experimental dedupe path).
- Test build wiring now includes compiler headers for engine tests on both:
  - `scripts/run_tests.ps1`
  - `build.sh` (when `RUN_TESTS=1`)
- Build scripts now compile/link the new IR unit:
  - `build.ps1`
  - `build.sh`
- Build scripts now compile/link the new optimizer unit:
  - `build.ps1`
  - `build.sh`

## [0.1.0] — 2026-04-22

### Added — Prompt 1 execution
- Monorepo layout per architecture §2.
- Apache-2.0 license.
- Public C ABI `engine/include/trinary/trinary.h` (symbols `trinary_v1_*`).
- Hand-written x86-64 AVX2 assembly kernels (MASM syntax, `ml64.exe`):
  - `trinary_v1_braincore_u8_avx2` — 8-bit LIF neuromorphic lattice update.
  - `trinary_v1_harding_gate_i16_avx2` — `(a·b) − (a⊕b)` over int16 arrays.
  - `trinary_v1_lattice_flip_avx2` — bit-packed lattice sign flip.
- Scalar C fallbacks for every asm kernel.
- CPUID-based load-time dispatch (`trinary_v1_init`).
- Aligned allocator with Windows large-page attempt + mmap/MADV_HUGEPAGE on POSIX.
- `.tri` v0 interpreter: lexer + recursive-descent parser + direct kernel dispatch.
  Handles `print`, `lattice: trit[N] = V`, `par spacetime:` blocks, `braincore`, comments.
- `trinary.exe` native CLI.
- TriPy Python package:
  - `tripy._core` C extension statically linked to engine.
  - `tripy run <file>`, `tripy braincore`, `tripy bench`, `tripy repl`.
  - `tripy filename.py` → runpy with `tripy` importable.
  - `tripy filename.tri` → direct compiler dispatch.
  - Typed stubs (`_core.pyi`), `py.typed` marker.
- Unit tests for kernels (C) and CLI (pytest).
- `build.ps1` orchestrator: compiles asm + C + links libtrinary.lib + trinary.exe + tripy._core.pyd.
- `.tri` example programs in `language/examples/`.
- `trinary-tripy-architecture.md` — living architecture contract.

### Fixed (vs legacy)
- `_core.cpp` missing `<chrono>` include — rewritten in C, no chrono used.
- Duplicate `from . import _core` in `tripy/__init__.py` — clean rewrite.
- `_core` rand-without-seed — replaced with seeded `trinary_v1_rng`.
- Assumption of AVX2 on every CPU — now CPUID-dispatched with scalar fallback.
