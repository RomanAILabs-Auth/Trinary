# Trinary

## 1. Hero section

Trinary is a dual-repo system built for one objective: maximum real-world throughput from hand-written machine code.  
The engine (`Trinary`) is pure x86-64/AArch64 assembly for hot kernels, with zero LLVM dependency and no libc in hot paths.  
The front-end (`TriPy`) and `.tri` language route into the same IR and dispatch table, so both developer paths hit the same optimized execution core with no feature-branching inside hot loops.

**Source of truth:** [`trinary-tripy-architecture.md`](./trinary-tripy-architecture.md)

## 2. Key Differentiators

- **Machine-code-first core:** hand-written assembly kernels with C glue only where required for ABI, dispatch, and platform runtime.
- **Dual front-ends, one engine:** `tripy file.py`, `tripy file.tri`, and native `trinary` CLI all converge on the same kernel backend.
- **Non-negotiable kernel focus:** neuromorphic `braincore_u8`, Harding gate `i16`, bitpacked lattice flip, Clifford rotor, and prime sieve.
- **No hot-path feature checks:** load-time dispatch resolves best variants once; hot loops execute direct function pointers.
- **Quality gates are enforced:** perf gate rejects regressions above **3%** against baseline for tracked kernels.
- **Deterministic engineering contract:** architecture and phase status are maintained as a living contract, not a loose roadmap.
- **Reproducible benchmark harness:** machine-generated JSON/Markdown outputs for both kernel-level and cross-language comparisons.

## 3. Current Performance

### Engine Kernel Baseline (Architecture Reference Host)

`build/bin/trinary-bench` with warmup + timed replicates, release mode.

| Kernel | Variant | Workload | Best seconds | Throughput |
|---|---|---|---:|---:|
| `braincore_u8` | `avx2` | 4 MiB x 2 buffers, 1000 iters | 0.593 | **6.75 GNeurons/s** |
| `harding_gate_i16` | `avx2` | 96 MiB, 64 iters | 0.451 | **2.38 GElements/s** |
| `lattice_flip` | `avx2` | 64 MiB bitpacked lattice, 64 iters | 0.504 | **68.2 Gbit/s** |
| `prime_sieve` | `scalar` | limit=20,000,000, 8 iters | 0.118 | **1350 MLimits/s** |

### Cross-Language Local Snapshot (Prime Sieve)

Prime limit: `5,000,000`, best-of-3, same machine.

| Runtime | Available | Best seconds | Throughput (M nums/s) | `pi(5,000,000)` |
|---|---:|---:|---:|---:|
| TriPy (`.tri` via CLI) | yes | 0.003334 | **1499.70** | 348513 |
| Trinary (native) | yes | 0.003692 | **1354.28** | 348513 |
| Go 1.26 (compiled) | yes | 0.028813 | 173.53 | 348513 |
| C++17 -O3 (compiled) | yes | 0.030871 | 161.97 | 348513 |
| Node.js 22 (pure) | yes | 0.039817 | 125.58 | 348513 |
| Python 3.13 (pure) | yes | 0.057928 | 86.31 | 348513 |
| Mojo | no | - | - | - |
| Rust (compiled) | no | - | - | - |

Raw artifacts:
- `benchmarks/elite_results.md`
- `benchmarks/elite_results.json`
- `benchmarks/elite_lang_bench.py`

## 4. Quick Start

### Windows (PowerShell)

```powershell
./build.ps1 -Config Release
./build/bin/trinary.exe --features
python -m tripy.cli --version
python -m tripy.cli "language/examples/prime.tri"
```

Install locally:

```powershell
./scripts/install_trinary.ps1 -Config Release
./scripts/install_tripy.ps1
```

### POSIX (Linux/macOS)

```bash
./build.sh Release
./build/bin/trinary --features
python -m tripy.cli --version
python -m tripy.cli "language/examples/prime.tri"
```

Install locally:

```bash
./scripts/install_trinary.sh
./scripts/install_tripy.sh
```

## 5. Project Status

- **Phase 0:** complete (foundation, structure, baseline quality gates)
- **Phase 1:** in progress (remaining machine-code kernel work + hardening tasks)
- **Current state:** engine + TriPy are functional, tested, and benchmarked with release checks in place.

For exact status, decisions, and open items, read:
- [`trinary-tripy-architecture.md`](./trinary-tripy-architecture.md)
- [`CHANGELOG.md`](./CHANGELOG.md)

## 6. Philosophy

Trinary is built on a strict rule: quality does not yield to speed-of-shipping. Kernel performance is earned through explicit machine-code work, measured with reproducible harnesses, and guarded by hard regression gates. Architecture is treated as contract, not commentary.

## 7. License

This repository is licensed under the **RomanAILabs Business Source License 1.1 (RBSL 1.1)**.

- Non-commercial research, education, and evaluation are permitted per license terms.
- Commercial use requires a separate written commercial license from RomanAILabs unless covered by the Additional Use Grant conditions in `LICENSE`.
- Each version automatically converts to **Apache-2.0** after the defined 3-year Change Date (or earlier at RomanAILabs' discretion).

Commercial licensing contact:
- `daniel@romanailabs.com`
- `romanailabs@gmail.com`
- `romanailabs.com`

## 8. Honored Collaborators

RomanAILabs honors collaboration with:
- Grok / xAI
- Gemini-Flash / Google
- ChatGPT-5.4 / OpenAI
- Cursor

To contribute:
- Follow `CONTRIBUTING.md` quality gates and coding standards.
- Run release checks before opening changes:
  - Windows: `./scripts/release_check.ps1 -RunTests`
  - POSIX: `./scripts/release_check.sh 1`
- Do not bypass performance gates for kernel/compiler path changes.

For commercial licensing, enterprise deployment, custom kernel development, or support agreements, contact:
- `daniel@romanailabs.com`
- `romanailabs@gmail.com`
- `romanailabs.com`
