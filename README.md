# Trinary

[![CI](https://github.com/RomanAILabs-Auth/Trinary/actions/workflows/ci.yml/badge.svg)](https://github.com/RomanAILabs-Auth/Trinary/actions/workflows/ci.yml)
[![Lint](https://github.com/RomanAILabs-Auth/Trinary/actions/workflows/lint.yml/badge.svg)](https://github.com/RomanAILabs-Auth/Trinary/actions/workflows/lint.yml)
[![License: RBSL 1.1](https://img.shields.io/badge/license-RBSL%201.1-111827.svg)](./LICENSE)
[![Python](https://img.shields.io/badge/python-3.12%2B-3776AB.svg?logo=python&logoColor=white)](./tripy/pyproject.toml)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-0A0A0A.svg)](./.github/workflows/ci.yml)

**Pure hand-written machine code in the hot path.**  
Trinary is an HPC-focused engine/runtime stack where kernel-critical execution is implemented in hand-written x86-64 / AArch64 assembly, dispatched once at init, and guarded by hard performance gates.

Copyright (c) RomanAILabs - Daniel Harding (GitHub: RomanAILabs-Auth)  
Honored collaborators: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor  
Contact: `daniel@romanailabs.com` | `romanailabs@gmail.com` | `romanailabs.com`

---

## Core Contract

- **Assembly-first execution model**: no LLVM dependency in the core path, no libc inside hot loops.
- **Dual front-ends, one engine contract**: `tripy <file.py>` and `tripy <file.tri>` converge on the same Trinary backend.
- **Load-time dispatch, zero feature branching in kernels**: CPU feature selection happens once via dispatch table init.
- **Strict performance policy**: any tracked kernel regression over **3%** fails the perf gate.
- **Living architecture governance**: all active scope, phase status, and decisions are tracked in [`trinary-tripy-architecture.md`](./trinary-tripy-architecture.md).

---

## Kernel Surface

Current kernel scope includes:
- `braincore_u8` (neuromorphic lattice)
- `harding_gate_i16`
- `lattice_flip` (bitpacked trit-lattice transform)
- `rotor_cl4` (Clifford rotor path)
- `prime_sieve`

---

## Benchmarks

### Engine Baseline (Reference Host)

Harness: `build/bin/trinary-bench` (warmup + timed replicates, release build).

| Kernel | Variant | Workload | Best seconds | Throughput |
|---|---|---|---:|---:|
| `braincore_u8` | `avx2` | 4 MiB x 2 buffers, 1000 iterations | 0.593 | **6.75 GNeurons/s** |
| `harding_gate_i16` | `avx2` | 96 MiB, 64 iterations | 0.451 | **2.38 GElements/s** |
| `lattice_flip` | `avx2` | 64 MiB bitpacked lattice, 64 iterations | 0.504 | **68.2 Gbit/s** |
| `prime_sieve` | `scalar` | limit = 20,000,000, 8 iterations | 0.118 | **1350 MLimits/s** |

### Cross-Language Prime Sieve Snapshot (Local Host)

Benchmark: prime limit `5,000,000`, best-of-3.  
Validation: all reported lanes match `pi(5,000,000) = 348513`.

| Runtime | Best seconds | Throughput (M nums/s) | Prime count |
|---|---:|---:|---:|
| TriPy (`.tri` via CLI) | 0.003334 | **1499.70** | 348513 |
| Trinary (native) | 0.003692 | **1354.28** | 348513 |
| Go 1.26 (compiled) | 0.028813 | 173.53 | 348513 |
| C++17 -O3 (compiled) | 0.030871 | 161.97 | 348513 |
| Node.js 22 (pure) | 0.039817 | 125.58 | 348513 |
| Python 3.13 (pure) | 0.057928 | 86.31 | 348513 |

Reproducibility artifacts:
- `benchmarks/elite_lang_bench.py`
- `benchmarks/elite_results.md`
- `benchmarks/elite_results.json`
- `benchmarks/bench_engine.c`
- `benchmarks/perf_gate.py`

Note: Mojo and Rust lanes are supported by the harness but omitted from this table when the local toolchains are unavailable.

---

## Quick Start

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

---

## Current Phase

- **Phase 0**: complete
- **Phase 1**: in progress
- Engine + TriPy paths are operational, tested, and benchmarked with release checks in place.

Roadmap, decisions, and compliance matrix:
- [`trinary-tripy-architecture.md`](./trinary-tripy-architecture.md)
- [`CHANGELOG.md`](./CHANGELOG.md)

---

## Engineering Quality Gates

- C format: `clang-format`
- C lint: `clang-tidy`
- Python lint/format: `ruff`
- Python typing: `mypy --strict`
- Tests: engine + TriPy suites
- Performance gate: **<=3%** regression tolerance for tracked kernels

Release verification:
- Windows: `./scripts/release_check.ps1 -RunTests`
- POSIX: `./scripts/release_check.sh 1`

---

## License

This repository uses the **RomanAILabs Business Source License 1.1 (RBSL 1.1)**.

- Non-commercial research, education, and evaluation are allowed under the license terms.
- Commercial use requires a separate written commercial license unless covered by the Additional Use Grant in [`LICENSE`](./LICENSE).
- Each version converts to **Apache License 2.0** after the defined 3-year Change Date (or earlier at RomanAILabs discretion).

Commercial and enterprise contact:
- `daniel@romanailabs.com`
- `romanailabs@gmail.com`
- `romanailabs.com`

---

## Honored Collaborators

- Grok / xAI
- Gemini-Flash / Google
- ChatGPT-5.4 / OpenAI
- Cursor
