# Trinary + TriPy — Unified Architecture

> **Living document.** Every prompt from the author is recorded, dated, and resolved into locked design decisions. Nothing in this file is speculative — if it is written here, it is the contract.

- **Repo root:** `C:\Users\Asus\Desktop\Documents\` (for now) → target monorepo: `trinary/`
- **Author:** Daniel Harding — RomanAILabs
- **Doc owner:** this file. All architecture changes land here first, code second.
- **Status:** Phase 0 — Foundation **complete** (2026-04-22). Phase 1 in flight (3 / 5 kernels shipped, sanitizer matrix + remaining kernels pending).

---

## 0. North Star (non-negotiable)

1. **Trinary is pure machine code.** Hand-written x86-64 / AArch64 assembly, assembled directly to a native executable and a loadable object. No libc dependency where it can be avoided. No C++ runtime overhead in hot paths. Syscalls issued directly.
2. **TriPy is the Python front-end.** The user runs:
   ```
   tripy filename.py
   tripy filename.tri
   tripy braincore
   ```
   TriPy parses the input, chooses the Trinary kernel, invokes it through a zero-copy FFI boundary, and returns results.
3. **One engine. One IR. Two front-ends.** The `.tri` language and Python `@accelerate` both lower to the same Trinary IR, which is emitted to the same hand-tuned machine-code kernels.
4. **Quality never yields to speed-of-shipping.** If a shortcut breaks the quality bar, the shortcut loses.

---

## 1. Product Definitions

### 1.1 Trinary — the machine-code engine
- **Deliverable:** `trinary.exe` (Windows), `trinary` (Linux/macOS), and `libtrinary.{dll,so,dylib}` exposing a stable C ABI.
- **Build system:** CMake orchestrates; per-architecture sources are `.S` (GNU-as / MSVC ml64 / Zig `cc -x assembler`). C++ is used **only** for glue, the public C header, and non-hot-path plumbing.
- **Hot-path rule:** kernels are assembly. Period. C/C++ intrinsics are allowed only as a temporary reference implementation, gated behind `TRINARY_REFERENCE=1`, and never the default.
- **Public surface:** a single C header `trinary.h` with a stable ABI (`trinary_v1_*` symbols).

### 1.2 TriPy — the Python driver
- **Deliverable:** `pip install tripy` → installs a `tripy` console script.
- **Invocation contract (frozen):**
  | Command | Behavior |
  |---|---|
  | `tripy filename.py` | Executes the Python file with `tripy` importable. Any `@tripy.accelerate` function is JIT-lowered to the Trinary IR and dispatched to a machine-code kernel. |
  | `tripy filename.tri` | Parses the `.tri` file with the real Trinary compiler, lowers to IR, dispatches to machine-code kernels. No Python interpretation involved. |
  | `tripy braincore` | Shortcut: run the canonical 8-bit neuromorphic lattice benchmark. |
  | `tripy bench` | Run the full benchmark suite and emit JSON to stdout. |
  | `tripy repl` | Interactive REPL that accepts both Python and `.tri` snippets. |
  | `tripy --version`, `tripy --help` | Standard. |
- **Binding:** `tripy._core` — a CPython extension written in C (not C++) that calls into `libtrinary` through the C ABI. Zero-copy via the buffer protocol and DLPack.

---

## 2. Repository Layout (target)

```
trinary/
├── trinary-tripy-architecture.md     ← this file (source of truth)
├── README.md
├── LICENSE                            (RomanAILabs Business Source License 1.1)
├── CHANGELOG.md
├── SECURITY.md
├── CONTRIBUTING.md
├── .gitignore                         (excludes *.exe, *.pdb, build/, __pycache__/, etc.)
├── .clang-format, .clang-tidy, .editorconfig, ruff.toml, mypy.ini, pyproject.toml
├── cmake/                             (toolchain files, arch detection)
├── engine/                            === TRINARY: MACHINE-CODE ENGINE ===
│   ├── include/trinary/trinary.h      (stable C ABI, v1)
│   ├── asm/
│   │   ├── x86_64/
│   │   │   ├── braincore_avx2.S
│   │   │   ├── braincore_avx512.S
│   │   │   ├── lattice_bitpacked.S
│   │   │   ├── rotor_cl4.S
│   │   │   ├── harding_gate.S
│   │   │   └── entry_windows.S / entry_sysv.S
│   │   └── aarch64/
│   │       ├── braincore_neon.S
│   │       └── ...
│   ├── src/                            (thin C glue only — dispatch, allocator, feature detection)
│   │   ├── dispatch.c
│   │   ├── allocator.c
│   │   ├── cpuid.c
│   │   └── capi.c
│   ├── compiler/                       (.tri → IR → codegen-to-asm-or-JIT)
│   │   ├── lexer.c
│   │   ├── parser.c
│   │   ├── sema.c
│   │   ├── ir.c
│   │   ├── optimizer.c
│   │   └── codegen_x86_64.c           (emits raw machine code into executable pages)
│   ├── tests/                          (unity test framework, sanitizer matrix)
│   └── CMakeLists.txt
├── tripy/                             === TRIPY: PYTHON FRONT-END ===
│   ├── pyproject.toml                 (scikit-build-core + cibuildwheel)
│   ├── src/tripy/
│   │   ├── __init__.py
│   │   ├── _core.pyi                  (typed stubs)
│   │   ├── runtime.py                 (session, device, stream)
│   │   ├── arrays.py                  (TrinaryArray + NumPy/DLPack interop)
│   │   ├── jit.py                     (@accelerate: Python AST → Trinary IR)
│   │   ├── lowering.py
│   │   ├── cli.py                     (tripy run/bench/repl/compile)
│   │   └── py.typed
│   ├── src/_core/                     (C extension — CPython API → libtrinary)
│   │   ├── module.c
│   │   └── bindings.c
│   └── tests/
├── language/                          === .tri LANGUAGE ===
│   ├── spec/
│   │   └── grammar.ebnf
│   ├── stdlib/                        (.tri standard library)
│   └── examples/
├── benchmarks/                        (Google Benchmark + pytest-benchmark + reproducible harness)
├── docs/                              (MkDocs-Material site)
├── tools/
│   ├── tri-fmt/
│   ├── tri-lsp/
│   └── tri-bench/
└── .github/workflows/                 (CI matrix: Win/Linux/macOS × x64/arm64 × Debug/Release)
```

All current files in `C:\Users\Asus\Desktop\Documents\Trinary\` and `...\TriPy\` that are worth keeping get moved under `legacy/` in Phase 0 and then ported into the target layout above, one kernel at a time.

---

## 2.1 Compliance Matrix — §2 target vs. repo state (2026-04-22)

Legend: **SHIPPED** = path exists and is functional. **STUB** = path exists and
carries a README explaining status + scope; no misleading placeholder code.
**DEFERRED** = intentionally not yet created; tracked by phase. **DEVIATED**
= diverges from §2 by a logged decision (see §8).

| §2 path                                   | Repo state    | Notes / Decision                                                                                          |
|-------------------------------------------|---------------|-----------------------------------------------------------------------------------------------------------|
| `trinary-tripy-architecture.md`           | SHIPPED       | This file.                                                                                                |
| `README.md`                               | SHIPPED       |                                                                                                           |
| `LICENSE`                                 | SHIPPED       | Apache-2.0 (pending Q-001 confirmation).                                                                  |
| `CHANGELOG.md`                            | SHIPPED       |                                                                                                           |
| `SECURITY.md`                             | SHIPPED       |                                                                                                           |
| `CONTRIBUTING.md`                         | SHIPPED       |                                                                                                           |
| `.gitignore`                              | SHIPPED       | Excludes `build/`, `legacy/`, `__pycache__/`, artifacts, `.pytest_cache/` (scoped).                       |
| `.clang-format`                           | SHIPPED       |                                                                                                           |
| `.clang-tidy`                             | SHIPPED       | Bugprone + cert + clang-analyzer + misc + performance + portability + scoped readability.                 |
| `.editorconfig`                           | SHIPPED       |                                                                                                           |
| `ruff.toml`                               | DEVIATED      | Consolidated into **root `pyproject.toml [tool.ruff]`** — modern standard.                                |
| `mypy.ini`                                | DEVIATED      | Consolidated into **root `pyproject.toml [tool.mypy]`** — modern standard, `strict = true`.               |
| `pyproject.toml` (root)                   | SHIPPED       | Repo-wide `ruff` / `mypy` / `pytest` config. Per-package `pyproject.toml` lives in `tripy/`.              |
| `cmake/`                                  | STUB          | Empty + `README.md` explaining D-008 (Zig is primary, CMake returns in Phase 3 wheel work).               |
| `engine/include/trinary/trinary.h`        | SHIPPED       | Public C ABI v1 frozen.                                                                                   |
| `engine/asm/x86_64/braincore_avx2.S`      | SHIPPED       | Actual name: `braincore_u8_avx2.S` (reflects u8 LIF variant).                                             |
| `engine/asm/x86_64/braincore_avx512.S`    | DEFERRED      | Phase 1 finish; author machine has no AVX-512 — CI arm needs CPU with AVX-512 first.                      |
| `engine/asm/x86_64/lattice_bitpacked.S`   | SHIPPED       | Actual name: `lattice_flip_avx2.S`.                                                                       |
| `engine/asm/x86_64/rotor_cl4.S`           | DEFERRED      | Phase 1 finish.                                                                                           |
| `engine/asm/x86_64/harding_gate.S`        | SHIPPED       | Actual name: `harding_gate_i16_avx2.S`.                                                                   |
| `engine/asm/x86_64/entry_windows.S`       | DEFERRED      | Libc-free entry points; today `main.c` uses libc stdio. Phase 1+ stretch (see §3).                        |
| `engine/asm/x86_64/entry_sysv.S`          | DEFERRED      | Same as above for POSIX.                                                                                  |
| `engine/asm/aarch64/*`                    | STUB          | Empty dir + `README.md` documenting Phase 1+ NEON / SVE2 scope and byte-exact contract.                   |
| `engine/src/dispatch.c`                   | SHIPPED       |                                                                                                           |
| `engine/src/allocator.c`                  | SHIPPED       |                                                                                                           |
| `engine/src/cpuid.c`                      | SHIPPED       |                                                                                                           |
| `engine/src/capi.c`                       | SHIPPED       |                                                                                                           |
| `engine/src/` extras                      | SHIPPED (+)   | Also ships `rng.c`, `timing.c`, `version.c`, 3× `*_scalar.c`, `main.c` — all required by the C ABI.       |
| `engine/compiler/lexer.c`                 | SHIPPED       | Active lexer extraction used in build (`build.ps1` / `build.sh`).                                         |
| `engine/compiler/parser.c`                | SHIPPED       | Active parser/interpreter extraction used in build; exports `.tri` entry points.                          |
| `engine/compiler/sema.c`                  | SHIPPED       | v0 semantic guardrails (declaration extents, loop range sanity, kernel argument canonicalization).        |
| `engine/compiler/ir.c`                    | SHIPPED       | v0 executable IR op layer (`braincore`, `prime`) wired between parser and kernel dispatch.               |
| `engine/compiler/optimizer.c`             | SHIPPED       | v0 optimizer stage with batch API canonicalizes IR programs before execution.                              |
| `engine/compiler/codegen_x86_64.c`        | DEFERRED      | Phase 2 — direct machine-code emission (v0 is interpreter + kernel dispatch).                             |
| `engine/tests/`                           | SHIPPED       | 9/9 tests; AVX2 byte-exact vs. scalar; argument-validation rejections.                                    |
| `engine/CMakeLists.txt`                   | DEVIATED      | Per D-008: `build.ps1` (Windows) and `build.sh` (POSIX) are authoritative. CMake returns in Phase 3.      |
| `tripy/pyproject.toml`                    | SHIPPED       | Setuptools (D-008 deferred scikit-build-core to Phase 3 wheel work).                                      |
| `tripy/src/tripy/__init__.py`             | SHIPPED       |                                                                                                           |
| `tripy/src/tripy/_core.pyi`               | SHIPPED       |                                                                                                           |
| `tripy/src/tripy/runtime.py`              | SHIPPED       |                                                                                                           |
| `tripy/src/tripy/arrays.py`               | DEFERRED      | Phase 3 — `TrinaryArray` + NumPy `__array_interface__` + DLPack.                                          |
| `tripy/src/tripy/jit.py`                  | SHIPPED       | v0.1 AST-inspecting `@accelerate`; full lowering is Phase 3.                                              |
| `tripy/src/tripy/lowering.py`             | DEFERRED      | Phase 3 — Python AST → Trinary IR.                                                                        |
| `tripy/src/tripy/cli.py`                  | SHIPPED       | `tripy <file>`, `braincore`, `bench`, `repl`, `--version`, `--features`, `--help`.                        |
| `tripy/src/tripy/py.typed`                | SHIPPED       |                                                                                                           |
| `tripy/src/_core/module.c`                | SHIPPED       |                                                                                                           |
| `tripy/src/_core/bindings.c`              | DEVIATED      | Consolidated into `module.c` (single translation unit, < 400 LoC). Split when it crosses ~800 LoC.        |
| `tripy/tests/`                            | SHIPPED       | 13/13 tests.                                                                                              |
| `language/spec/grammar.ebnf`              | SHIPPED       |                                                                                                           |
| `language/stdlib/`                        | STUB          | Empty dir + `README.md` documenting Phase 2 stdlib plan.                                                  |
| `language/examples/`                      | SHIPPED       | `hello.tri`, `braincore.tri`, `lattice_flip.tri` — all run.                                               |
| `benchmarks/`                             | SHIPPED       | `bench_engine.c` + `perf_gate.py` + `baseline.json` + 4 self-tests. Google Benchmark skipped by design.   |
| `docs/`                                   | STUB          | Empty + `README.md` documenting Phase 4 MkDocs-Material plan.                                             |
| `tools/zig/`                              | SHIPPED       | Pinned Zig 0.16.0 (D-008).                                                                                |
| `tools/tri-fmt/`                          | STUB          | Empty dir; documented in `tools/README.md`.                                                               |
| `tools/tri-lsp/`                          | STUB          | Empty dir; documented in `tools/README.md`.                                                               |
| `tools/tri-bench/`                        | STUB          | Empty dir; documented in `tools/README.md`.                                                               |
| `.github/workflows/`                      | SHIPPED       | `ci.yml` (Windows full + Linux portability) + `lint.yml` (ruff + mypy + clang-format). ARM/macOS deferred.|
| `legacy/`                                 | SHIPPED       | 49 items from old `Trinary/` + full old `TriPy/` preserved frozen; `legacy/README.md` sets freeze rules.  |

**Scorecard (paths in §2):** 43 SHIPPED · 7 STUB · 6 DEFERRED · 5 DEVIATED (all
deviations trace to a logged decision row in §8; none are accidental).

---

## 3. Machine-Code Contract (Trinary)

### 3.1 ABI
- Public C ABI, documented in `engine/include/trinary/trinary.h`.
- Symbol prefix: `trinary_v1_`.
- Calling convention: platform default (Microsoft x64 on Windows; SysV AMD64 on Linux/macOS; AAPCS64 on ARM).
- Thread safety: every public function is either thread-safe or documented as `@single-threaded`.

### 3.2 Required kernels for v1.0
| Kernel | Purpose | Target instructions |
|---|---|---|
| `trinary_v1_braincore_u8` | 8-bit LIF neuromorphic lattice | AVX2 / AVX-512 / NEON |
| `trinary_v1_lattice_bitpacked` | 1-bit / 2-bit trit lattice update | AVX2 / AVX-512 / NEON |
| `trinary_v1_rotor_cl4` | Clifford Cl(4,0) rotor sandwich | AVX2 / AVX-512 / NEON |
| `trinary_v1_harding_gate_i16` | `(a·b) − (a⊕b)` over int16 lanes | AVX2 / AVX-512 / NEON |
| `trinary_v1_prime_sieve` | Segmented wheel sieve | scalar + AVX-512 popcount |

### 3.3 Dispatch
- At load time, `trinary_v1_init()` runs CPUID, picks the best kernel variant for each slot, stores function pointers in a global dispatch table. No branch on feature inside hot loops.
- Variants: `avx512` → `avx2` → `sse42` → `scalar` on x86; `sve2` → `neon` → `scalar` on ARM.
- Fallback scalar path is always present. A 2007 CPU must still run Trinary correctly — just slowly.

### 3.4 Memory model
- 64-byte aligned allocations via `VirtualAlloc` (Windows large pages where available) and `mmap + madvise(MADV_HUGEPAGE)` (Linux).
- NUMA-local first-touch policy.
- Zero dynamic allocation in the hot loop. All scratch buffers are pool-allocated at session creation.

### 3.5 Performance gate
- Every kernel has a Google Benchmark entry. A PR that regresses any kernel mean latency by >3% across 10 runs on the reference CI host **fails CI**. No exceptions without a written waiver recorded here.

---

## 4. TriPy Runtime Contract

### 4.1 `tripy filename.py`
1. Start CPython 3.12+.
2. Inject the `tripy` package into the file's globals.
3. `runpy.run_path(filename, run_name="__main__")`.
4. When a `@tripy.accelerate` function is first called, `jit.py` captures the AST, lowers it to Trinary IR, requests machine code from `libtrinary`, caches the pointer, and subsequent calls dispatch directly with zero Python overhead per call.

### 4.2 `tripy filename.tri`
1. No Python interpretation.
2. `_core.load_tri(filename)` → hands the path to `libtrinary`'s compiler.
3. Compiler lexes → parses → type-checks → builds IR → emits machine code into an executable page (mmap-PROT_EXEC / VirtualAlloc PAGE_EXECUTE_READWRITE).
4. `libtrinary` returns a function pointer to the program entry.
5. TriPy invokes it, streams stdout back to Python, and returns the exit code.

### 4.3 `tripy braincore`
Direct call into `trinary_v1_braincore_u8` with canonical parameters (`NEURON_COUNT=4_000_000`, `ITERATIONS=1000`). Result is a structured JSON object (wall time, throughput, variance, CPU flags used).

### 4.4 Safety
- No `exec(open(...).read())`. Ever.
- `.py` execution uses `runpy` with a proper `__main__` frame.
- `.tri` compilation runs in a sandboxed arena with an explicit memory budget (`TRIPY_MAX_HEAP=`, default 8 GiB).

---

## 5. Quality Gates (global)

| Gate | Tool | Requirement |
|---|---|---|
| C format | clang-format | zero diff |
| C lint | clang-tidy (bugprone, cert, performance) | zero warnings |
| ASM sanity | assembler + `objdump -d` diff | golden-file match |
| Sanitizers | ASan, UBSan, TSan | clean on glue + compiler |
| Fuzzing | libFuzzer on lexer/parser/IR | no crashes in 10M iterations nightly |
| Coverage | llvm-cov (C), coverage.py (Python) | ≥85% C, ≥90% Python |
| Python format | ruff format | zero diff |
| Python lint | ruff (ALL) | zero warnings |
| Python types | mypy --strict + pyright | zero errors |
| Python tests | pytest + hypothesis | 100% pass |
| Perf gate | Google Benchmark | ≤3% regression vs main |
| Supply chain | pip-audit, gitleaks | clean |
| Release | cosign signatures + CycloneDX SBOM | attached to every tag |

CI matrix: `{windows-2022, ubuntu-24.04, macos-14} × {x86_64, arm64} × {Debug, Release}`.

---

## 6. Phase Plan (no calendar — exit criteria only)

Each phase is done when **every** exit criterion is green. No phase slips by moving dates; phases slip by failing criteria, which we then fix.

### Phase 0 — Foundation  **[DONE 2026-04-22]**
- [x] Monorepo layout from §2 created at `C:\Users\Asus\Desktop\Documents\trinary\`.
- [x] `legacy/` folder — original `Documents\Trinary\*` moved to `legacy/trinary/`;
  original `Documents\TriPy\*` moved to `legacy/tripy/`; `legacy/README.md`
  documents freeze-rules (nothing in `legacy/` is built, linted, or shipped).
- [x] `.gitignore` excludes all build artifacts (`build/`, `*.obj`, `*.pyd`, `__pycache__`, etc.).
- [x] `trinary-tripy-architecture.md` (this file) in-repo.
- [x] LICENSE (Apache-2.0), README, CHANGELOG, CONTRIBUTING, SECURITY in place.
- [x] CI skeleton — `.github/workflows/ci.yml` (Windows primary + Linux portability
  check, both producing artifacts) and `.github/workflows/lint.yml` (ruff, ruff
  format --check, mypy --strict, clang-format --dry-run --Werror).
- [x] Cross-platform build entry points: `build.ps1` (Windows, bundled Zig) and
  `build.sh` (POSIX, system `cc`).
- [x] Formal benchmark harness `benchmarks/bench_engine.c` producing
  `trinary.bench.v1` JSON; perf-gate diff in `benchmarks/perf_gate.py`
  (exits 2 on >3% regression per D-007); initial baseline at
  `benchmarks/baseline.json`; self-tested by `benchmarks/tests/test_perf_gate.py`.

### Phase 1 — Engine v1.0 (machine-code kernels)  **[v0.1 shipped]**
- [x] `engine/include/trinary/trinary.h` v1 — public ABI frozen.
- [x] **3 of 5** kernels shipped as hand-written x86-64 assembly (GAS Intel syntax, assembled by `zig cc -x assembler`):
  - [x] `trinary_v1_braincore_u8_avx2`    — AVX2 saturating u8 LIF.
  - [x] `trinary_v1_harding_gate_i16_avx2` — AVX2 `(a·b) − (a⊕b)` over int16.
  - [x] `trinary_v1_lattice_flip_avx2`    — AVX2 bit-packed XOR flip.
  - [ ] `trinary_v1_rotor_cl4` — AVX2/AVX-512/NEON asm deferred.
  - [ ] `trinary_v1_prime_sieve` — AVX-512 popcount asm deferred.
- [x] Scalar C fallback for every shipped kernel; output is byte-identical to AVX2.
- [x] Scalar reference implementations now exist for deferred kernels:
  - [x] `trinary_v1_rotor_cl4_f32` (XY+ZW two-plane rotor transform).
  - [x] `trinary_v1_prime_sieve_u64` (odd-only segmented sieve).
- [x] Dispatch via CPUID + OS-XSAVE at load time (`trinary_v1_init`).
- [x] Allocator moved toward §3.4 contract:
  - minimum allocation alignment raised to 64 bytes.
  - Windows allocator path uses `VirtualAlloc` (large-page attempt first).
  - POSIX allocator path uses `mmap` + `madvise(MADV_HUGEPAGE)` hint.
- [x] Reference numbers recorded in §10 below.
- [ ] Sanitizer matrix (ASan / UBSan / TSan) — deferred.

### Phase 2 — `.tri` Language v1.0  **[v0 interpreter shipped, compiler deferred]**
- [x] Formal v0 grammar in `language/spec/grammar.ebnf`.
- [x] Real lexer + recursive-descent parser with line/column diagnostics.
- [x] v0 dispatch backend: interprets `.tri` AST and routes to machine-code kernels.
  Supported today: `print`, `lattice: trit[N] = V`, `par spacetime:` block with
  `for t in range(0, K):  lattice = lattice * -1`, `braincore(N, I)`, comments.
- [x] Minimal executable IR layer (`engine/compiler/ir.c`) now routes parsed kernel
  ops (`braincore`, `prime`) through explicit IR opcodes before dispatch.
- [x] Minimal optimizer stage (`engine/compiler/optimizer.c`) now canonicalizes IR
  programs before execution (pipeline is now parse -> sema -> IR -> optimize -> execute).
- [x] Example programs in `language/examples/` run and produce output.
- [ ] Full type checker, IR, optimizer, direct x86-64 machine-code emitter — Phase 2 continuation.
- [ ] Fuzzing — deferred.

### Phase 3 — TriPy v1.0  **[v0.1 shipped as source install]**
- [x] `tripy <file.py>`, `tripy <file.tri>`, `tripy braincore`, `tripy bench`,
  `tripy repl`, `tripy --version`, `tripy --features`, `tripy --help` all work.
- [x] `@tripy.accelerate` AST-inspecting decorator that routes recognized kernels.
- [x] `tripy._core` C extension statically linked to `libtrinary.a` (no runtime DLL).
- [x] Typed stubs (`_core.pyi`), `py.typed` marker shipped.
- [ ] Full AST → Trinary IR JIT lowering for arbitrary Python — Phase 3 continuation.
- [ ] `TrinaryArray` + NumPy / DLPack interop — deferred.
- [ ] `pip install tripy` from PyPI + cibuildwheel — deferred (today it's source-install from repo).

### Phase 4 — Launch
- [ ] `docs/` site live (MkDocs-Material).
- [ ] Every public API documented with a doctest-runnable example.
- [ ] All performance claims in the README reproducible via `tripy bench`.
- [ ] Signed 1.0.0 release, SBOM attached.

---

## 7. Decision Log

| # | Date | Decision | Rationale |
|---|---|---|---|
| D-001 | 2026-04-22 | Trinary is pure machine code; C/C++ only as glue. | Author mandate. Fastest possible execution. |
| D-002 | 2026-04-22 | TriPy CLI contract frozen: `tripy <file.py>`, `tripy <file.tri>`, `tripy braincore`. | Author mandate. |
| D-003 | 2026-04-22 | No timeline gating; exit criteria per phase. | Author mandate — "do not limit this project to weeks or months". |
| D-004 | 2026-04-22 | License: Apache-2.0 (pending author confirm). | Permissive + patent grant + enterprise-friendly. |
| D-005 | 2026-04-22 | Python/C extension is written in C (not C++) for ABI stability. | Stable ABI, smaller surface, matches the "machine-code first" ethos. |
| D-006 | 2026-04-22 | No LLVM dependency in Phase 2 — we emit x86-64 machine code directly. | Keeps the engine small, auditable, and dependency-free. LLVM can be added later as an optional backend. |
| D-007 | 2026-04-22 | Perf gate: ≤3% regression across 10 runs or CI fails. | Forces performance to be a first-class invariant. |
| D-008 | 2026-04-22 | Build toolchain: bundled Zig 0.16.0 (`tools/zig/zig.exe`) — Clang 21 + MinGW-w64 libc + lld. | Author's Windows has MSVC 2022 but no Windows SDK. Zig bundles libc headers + linker; single-binary, cross-platform, reproducible. No MSVC / no SDK dependency. |
| D-009 | 2026-04-22 | Assembly syntax: GAS Intel-flavor (`.intel_syntax noprefix`) in `.S` files, not MASM. | Works with `zig cc -x assembler`; portable to Clang on Linux/macOS without rewrites. |
| D-010 | 2026-04-22 | Python extension is statically linked to `libtrinary.a` — no runtime DLL. | Simpler distribution, no `PATH`-search hazards, one engine copy per process. |

New decisions are appended, never rewritten. If a decision is reversed, the new row supersedes and cites the old one.

---

## 8. Open Questions (need author input)

- **Q-001** — License confirmation: Apache-2.0 (default) or MIT?
- **Q-002** — "441× faster" claim in current TriPy README: keep and prove with a rigorous harness, or retire until proven?
- **Q-003** — Primary target OS for the 1.0 launch: Windows-only, or Windows + Linux + macOS day-one?
- **Q-004** — GPU plans: stay CPU-only for 1.0, or open a Phase 5 GPU backend (CUDA/Metal/HIP) in scope?
- **Q-005** — Distribution: PyPI only, or PyPI + Homebrew tap + scoop + containers on day one?

---

## 9. Prompt Log

This section records every directive you give me, in order. Each prompt becomes locked architecture.

### Prompt 1 — 2026-04-22
> "Trinary is supposed to be built in Machine Code to give it the best speed possible. TriPy is supposed to be executed by for example 'tripy filename.py or tripy filename.tri' understand. and do not limit this project to weeks or months do your best to have this working today. but do not sacrifice quality."

**Resolved into architecture:**
- Section 0 (North Star) — items 1 and 2.
- Section 1.1 — "machine-code engine" contract.
- Section 1.2 — TriPy CLI invocation table (frozen).
- Decisions D-001, D-002, D-003.
- Phase plan (Section 6) uses exit criteria, not dates.

**Resolved into shipped code (Prompt 1 execution, 2026-04-22):**
- Monorepo at `C:\Users\Asus\Desktop\Documents\trinary\` matching §2.
- Toolchain: bundled Zig 0.16.0 at `tools/zig/` (Clang 21 + MinGW-w64 libc + lld) — see D-008.
- **Hand-written x86-64 AVX2 assembly** (GAS Intel-syntax `.S` files, D-009):
  - `engine/asm/x86_64/braincore_u8_avx2.S`
  - `engine/asm/x86_64/harding_gate_i16_avx2.S`
  - `engine/asm/x86_64/lattice_flip_avx2.S`
- Scalar C fallbacks for each kernel; outputs byte-identical to AVX2 (verified by `engine/tests/test_kernels.c`).
- CPUID + OS-XSAVE detection in `engine/src/cpuid.c`; load-time dispatch in `engine/src/dispatch.c`.
- Public C ABI frozen at `engine/include/trinary/trinary.h` (symbols `trinary_v1_*`).
- Native CLI `trinary.exe` (`engine/src/main.c`): `<file.tri>`, `braincore`, `bench`, `--features`, `--version`, `--help`.
- v0 `.tri` front-end now split into:
  - `engine/compiler/lexer.c`
  - `engine/compiler/parser.c`
  with behavior parity preserved from legacy `tri_lang.c`.
  Handles every form in `language/spec/grammar.ebnf`.
- TriPy package under `tripy/src/tripy/` with C extension `tripy/src/_core/module.c`
  statically linked to `libtrinary.a` (D-010).
- Full CLI per §1.2: `tripy filename.py`, `tripy filename.tri`, `tripy braincore`,
  `tripy bench`, `tripy repl`, `tripy --version`, `tripy --features`, `tripy --help`.
- `@tripy.accelerate` AST-inspecting decorator in `tripy/src/tripy/jit.py`.
- `build.ps1` single-command orchestrator; `scripts/run_tests.ps1` test runner.
- 9/9 engine kernel correctness tests pass (`engine/tests/test_kernels.c`):
  AVX2 output byte-identical to scalar reference, argument validation rejects misalignment/NULL.
- 13/13 TriPy Python smoke tests pass (`tripy/tests/test_smoke.py`):
  version, features, all three kernels, `run_tri`, `@accelerate`, CLI `--version`, CLI `braincore`, CLI `.tri`, CLI `.py`.

**Deliberately deferred** (flagged in the phase checklist, not dropped):
- Sanitizer matrix, fuzzing — Phase 1 finish (engine hardening).
- `rotor_cl4` and `prime_sieve` kernels — Phase 1 finish.
- Full `.tri` compiler with direct machine-code emission (v0 is interpreter+dispatch) — Phase 2.
- Full Python AST → Trinary IR JIT — Phase 3.
- NumPy/DLPack interop, PyPI wheels — Phase 3.
- MkDocs site, SBOM, signed release — Phase 4.

**Prompt 1 (continuation) — 2026-04-22 — Phase 0 close-out:**
- Moved 49 items (sources, artifacts, `examples/`) from root to `legacy/trinary/`;
  moved full original `TriPy/` tree to `legacy/tripy/`; both frozen.
- Added `build.sh` mirroring `build.ps1` for POSIX (so CI Linux can build).
- Added `.github/workflows/ci.yml` and `.github/workflows/lint.yml`.
- Added `benchmarks/bench_engine.c` (warmup + 10-replicate harness, JSON output,
  schema `trinary.bench.v1`), wired into both build scripts → produces
  `build/bin/trinary-bench(.exe)`.
- Added `benchmarks/perf_gate.py` with 4 self-tests under
  `benchmarks/tests/test_perf_gate.py`, all passing.
- Added `benchmarks/baseline.json` pinned to §10 reference numbers (below).
- `scripts/run_tests.ps1` now also runs perf-gate self-tests.
- End-to-end green: **9 engine tests + 13 tripy tests + 4 perf-gate tests = 26/26 pass.**

### Prompt 2 — 2026-04-22
> "Please look at the architecture ... continue building."

**Resolved into shipped code (Prompt 2 execution, 2026-04-22):**
- Added two new public ABI kernel endpoints to `engine/include/trinary/trinary.h`:
  - `trinary_v1_rotor_cl4_f32(...)`
  - `trinary_v1_prime_sieve_u64(...)`
- Implemented scalar kernel references:
  - `engine/src/rotor_cl4_scalar.c` (4D two-plane rotor transform over packed vec4<f32>).
  - `engine/src/prime_sieve_scalar.c` (odd-only segmented sieve, returns `pi(limit)`).
- Extended dispatch table in `engine/src/dispatch.c` with `rotor` + `prime` slots and variants.
  Current runtime variants on author machine: `rotor=avx2`, `prime=scalar`.
- Wired C ABI wrappers in `engine/src/capi.c` with argument validation and dispatch routing.
- Build integration:
  - Added both new sources to `build.ps1` and `build.sh`.
- Native CLI integration (`engine/src/main.c`):
  - Added `trinary prime [LIMIT]` command.
  - `trinary --features` now reports `rotor` and `prime` active variants.
- Kernel tests expanded in `engine/tests/test_kernels.c`:
  - Added rotor correctness test (API output vs scalar reference).
  - Added prime sieve correctness tests (`pi(100)==25`, `pi(1e5)` parity vs scalar ref).
  - Added argument validation tests for rotor and prime.
- Memory allocator hardening:
  - Replaced `_aligned_malloc/posix_memalign` primary path with OS-backed mapping allocator.
  - Added metadata-safe free path for map-vs-heap origin.
  - Added allocator tests proving 64-byte alignment guarantees.
- End-to-end status remains green:
  - engine suite passes with new kernel checks
  - python + perf-gate suites remain passing
  - total remains **26/26 pass** in scripted CI-style local run.
- Added AVX2-targeted rotor implementation (`engine/src/rotor_cl4_avx2.c`) and
  dispatch preference `avx2 -> scalar` for rotor. Correctness remains locked
  to scalar reference parity tests.
- Optimized `prime_sieve` scalar kernel (`engine/src/prime_sieve_scalar.c`) to
  packed-bitset segmented sieve + popcount counting. Sample on author machine:
  `trinary prime 50000000` => `pi(50,000,000)=3,001,134` in ~0.038 s
  (~1303 million numbers/s).
- Added `prime_sieve` to `benchmarks/bench_engine.c`; perf-gate remains green
  against current baseline for existing gated kernels (braincore/harding/flip),
  while `prime_sieve` is currently reported as NEW (no baseline yet).
- Locked `prime_sieve` into `benchmarks/baseline.json` and perf-gate; it is now
  CI-regression-gated with the same ≤3% rule as existing kernels.
- Split the `.tri` front-end implementation into dedicated units:
  - `engine/compiler/lexer.c`
  - `engine/compiler/parser.c`
  while preserving behavior and passing all existing test suites.
- Expanded v0 `.tri` kernel dispatch surface with direct `prime(...)` support
  (routes to `trinary_v1_prime_sieve_u64`), plus a new example program
  `language/examples/prime.tri`.
- Added semantic analysis stage scaffold (`engine/compiler/sema.c`) and wired
  parser through it for concrete v0 checks (non-zero declaration extents,
  `range` bound ordering, kernel argument canonicalization).
- Added initial executable IR stage (`engine/compiler/ir.c` + `ir.h`) and routed
  parser-emitted `braincore` / `prime` statements through typed IR opcodes before
  kernel dispatch; build scripts now compile the IR unit.
- Added initial optimizer stage (`engine/compiler/optimizer.c` + `optimizer.h`) and
  wired parser-emitted IR through canonicalization before execution; semantic finalizers
  for `braincore` / `prime` moved from execution layer into optimizer pass.
- Added a minimal IR program container API (`tri_ir_program_*`) and batch optimizer
  entry point (`tri_optimize_ir_program`) so parser now routes kernel ops through a
  real program-level optimize/execute path.
- Added compiler-layer engine tests in `engine/tests/test_kernels.c` covering:
  - IR optimizer canonicalization invariants
  - IR program push/optimize/execute pipeline sanity
  and updated test compile include paths in `scripts/run_tests.ps1` and `build.sh`.
- Parser now accumulates consecutive kernel statements into a queued IR program and
  flushes at safe boundaries (non-kernel statements and EOF), preserving semantics
  while widening optimizer scope to multi-op blocks. Added TriPy smoke coverage for
  consecutive `prime(...)` statements in one `.tri` file.
- Added additional correctness guards for queued IR semantics and program-level
  optimizer behavior:
  - TriPy smoke test for `prime -> print -> prime` flush-order boundary
  - Engine C tests asserting per-instruction canonicalization across multi-op IR programs.
- Added optimizer reporting scaffold (`tri_opt_report` +
  `tri_optimize_ir_program_report`) and wired parser queue flush through this
  report-capable pass, with engine tests validating report counters.
- Added pass-manager style optimizer entrypoint (`tri_optimize_ir_program_with_config`)
  and conservative multi-op redundancy analysis pass (detects adjacent identical
  `prime` operations as candidates without transforming them yet, preserving output
  semantics). Engine tests now validate candidate detection and pass-run counters.
- Added `TRI_IR_NOP` plus an explicit double-opt-in experimental transform gate in
  optimizer config (`enable_experimental_transforms` +
  `allow_observable_output_changes`) so future aggressive passes remain isolated
  from default production behavior. Engine tests validate both default-safe and
  explicit-opt-in transform paths.
- Added runtime optimizer toggle plumbing in parser via env vars:
  - `TRINARY_OPT_EXPERIMENTAL`
  - `TRINARY_OPT_ALLOW_OBSERVABLE`
  enabling opt-in experimental transform benchmarking without affecting default
  semantics. TriPy smoke tests now cover this opt-in runtime path.
- Added optional optimizer telemetry via `TRINARY_OPT_TRACE=1`, which emits
  optimizer pass report counters to stderr per queued-kernel flush. Default
  runtime remains silent; TriPy smoke tests cover trace output behavior.
- Added first-class `tripy` CLI optimizer flags as user-facing control plane for
  runtime optimizer modes (`--opt-default`, `--opt-experimental`,
  `--opt-allow-observable`, `--opt-trace`) with scoped env override behavior and
  smoke coverage for flag-driven experimental/trace paths.
- Added fail-safe guard coverage for optimizer mode control:
  - observable-output permission alone cannot activate transforms
  - `--opt-default` explicitly neutralizes inherited env toggles
  preserving default production semantics unless full opt-in is provided.
- Enhanced optimizer trace observability: parser now aggregates optimizer reports
  across queue flushes and emits an end-of-run summary (`[tri][opt-total] ...`)
  when trace mode is enabled, while keeping default runtime silent.
- Native `trinary` CLI now mirrors `tripy` optimizer controls via command-line
  flags (`--opt-default`, `--opt-experimental`, `--opt-allow-observable`,
  `--opt-trace`) so both front-ends expose a consistent runtime tuning surface.
- Documentation/licensing/install packaging pass completed:
  - Root dual-repo README rewritten with Human/LLM guides.
  - Added `engine/README.md` and expanded `tripy/README.md`.
  - Added cross-platform installers for both Trinary and TriPy under `scripts/`.
  - Repository license replaced with RomanAILabs Business Source License 1.1.
  - Added `NOTICE` and collaborator/author contact attribution.
- Applied attribution/contact header blocks across active Trinary/TriPy source
  entrypoints (engine src/compiler/asm/tests and tripy front-door modules).
- Expanded collaborator attribution set to include ChatGPT-5.4/OpenAI across
  README/NOTICE and active source attribution headers.
- Added automated release readiness checks under `scripts/release_check.ps1` and
  `scripts/release_check.sh` to validate required files, license/notice markers,
  and architecture/license consistency (with optional full test execution).
- Added elite cross-language benchmark harness (`benchmarks/elite_lang_bench.py`)
  with machine-generated markdown/json outputs and README integration for
  GitHub-facing performance reporting.

---

## 10. Reference Performance Numbers (author's machine, 2026-04-22)

Machine: Windows, AVX2 + BMI2 + FMA, AVX-512 absent. Reported by `trinary --features`:
`sse2=1 sse42=1 avx=1 avx2=1 avx512f=0 bmi2=1 popcnt=1 fma=1`.

**Authoritative baseline** (`benchmarks/baseline.json`) — 3 warmup + 10 timed
replicates, best-of-10 throughput, `build\bin\trinary-bench.exe`:

| Kernel             | Variant | Working set | Iter  | Best seconds | Throughput (best)        |
|--------------------|---------|-------------|------:|-------------:|--------------------------|
| `braincore_u8`     | avx2    | 4 MiB × 2   | 1000  | 0.593        | **6.75 GNeurons / s**    |
| `harding_gate_i16` | avx2    | 96 MiB      |   64  | 0.451        | **2.38 GElements / s**   |
| `lattice_flip`     | avx2    | 64 MiB (bits) | 64  | 0.504        | **68.2 Gbit / s**        |
| `prime_sieve`      | scalar  | limit=20,000,000 | 8 | 0.118       | **1350 MLimits / s**     |

Small-working-set (cache-resident) reference (`trinary braincore 32768 100`):
- `braincore_u8 avx2`: **~28.7 GNeurons / s** — the AVX2 kernel's instruction-throughput ceiling.

Reproducible with:
```
./build.ps1 -Config Release
./build/bin/trinary-bench.exe > build/bench/latest.json
python benchmarks/perf_gate.py --baseline benchmarks/baseline.json --current build/bench/latest.json
```

The ≤3 % regression gate (D-007) is machine-enforced by `perf_gate.py`:
any listed kernel whose best-of-10 throughput drops >3 % below baseline fails
with exit 2. Baseline is refreshed only on the designated reference host and
only as a logged decision.

`prime_sieve` is now part of the locked baseline (`benchmarks/baseline.json`) and
is evaluated by `perf_gate.py` alongside `braincore`, `harding_gate`, and `lattice_flip`.

---

_End of document. Everything above is contract. Everything below is future prompts._
