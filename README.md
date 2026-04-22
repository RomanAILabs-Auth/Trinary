# Trinary Dual Repo (Trinary + TriPy)

Trinary is a machine-code-first compute engine. TriPy is the Python front-end that drives the same kernels with a clean developer interface.

This repository is optimized for:
- low overhead kernel dispatch
- deterministic performance workflows
- clear ABI boundaries between engine and front-end
- practical use by both humans and coding agents/LLMs

## Core Components

- `engine/` (`Trinary`): kernel implementations, dispatch, allocator, C ABI, `.tri` frontend/compiler path
- `tripy/` (`TriPy`): Python package and CLI (`tripy`) bound to `libtrinary`
- `language/`: `.tri` examples and grammar
- `benchmarks/`: reproducible performance harness and perf gate tools
- `trinary-tripy-architecture.md`: the living source-of-truth architecture contract

## Quick Start

### Windows (PowerShell)

```powershell
./build.ps1 -Config Release
./build/bin/trinary.exe --features
python -m tripy.cli --version
python -m tripy.cli "language/examples/prime.tri"
```

### POSIX (Linux/macOS)

```bash
./build.sh Release
./build/bin/trinary --features
python -m tripy.cli --version
python -m tripy.cli "language/examples/prime.tri"
```

## Installers

Use the repository installers to install binaries/scripts in a user-local location.

- Windows:
  - `scripts/install_trinary.ps1`
  - `scripts/install_tripy.ps1`
- POSIX:
  - `scripts/install_trinary.sh`
  - `scripts/install_tripy.sh`

These scripts install to user-owned paths and update shell profile/PATH guidance where needed.

## Release Check

Use the release check scripts to verify legal/docs/install readiness.

- Windows: `./scripts/release_check.ps1`
- POSIX: `./scripts/release_check.sh`

Optional full validation:
- Windows: `./scripts/release_check.ps1 -RunTests`
- POSIX: `./scripts/release_check.sh 1`

## Elite Benchmark (Local Snapshot)

Benchmark kernel: prime counting to `5,000,000`, `3` runs each, best time reported.

| Runtime | Available | Best seconds | Throughput (M nums/s) | pi(limit) |
|---|---:|---:|---:|---:|
| TriPy (.tri via CLI) | yes | 0.003334 | 1499.70 | 348513 |
| Trinary (native) | yes | 0.003692 | 1354.28 | 348513 |
| Go 1.26 (compiled) | yes | 0.028813 | 173.53 | 348513 |
| C++17 -O3 (compiled) | yes | 0.030871 | 161.97 | 348513 |
| Node.js 22 (pure) | yes | 0.039817 | 125.58 | 348513 |
| Python 3.13 (pure) | yes | 0.057928 | 86.31 | 348513 |
| Mojo | no | - | - | - |
| Rust (compiled) | no | - | - | - |

Raw benchmark outputs:
- `benchmarks/elite_results.md`
- `benchmarks/elite_results.json`

Re-run locally:

```powershell
python benchmarks/elite_lang_bench.py
```

## How To Code (Human + LLM Guide)

### 1) Keep hot paths minimal

- Prefer work in `engine/src/dispatch.c`, kernels, and ABI wrappers over high-level indirection.
- Avoid hidden allocations in kernel execution paths.
- Keep scalar reference parity locked when introducing SIMD/ASM changes.

### 2) Respect contracts first

- C ABI: `engine/include/trinary/trinary.h`
- `.tri` semantics + architecture: `trinary-tripy-architecture.md`
- CLI contract: `tripy/src/tripy/cli.py` and `engine/src/main.c`

### 3) Change safely

- Write/update tests for every behavior change.
- Keep optimizer experimental features opt-in and default-safe.
- Never regress default output semantics unless explicitly authorized.

### 4) Always validate

- Build and test:
  - `./build.ps1 -Config Release -Tests` (Windows)
  - `./build.sh Release 1` (POSIX)
- Run perf harness and gate checks when touching kernel/compiler hot paths.

## Collaborators and Attribution

RomanAILabs honors collaboration with:
- Grok / xAI
- Gemini-Flash / Google
- ChatGPT-5.4 / OpenAI
- Cursor

Primary author:
- Daniel Harding (RomanAILabs)
- Email: `daniel@romanailabs.com`, `romanailabs@gmail.com`
- Website: `romanailabs.com`

## License

This repository is licensed under the RomanAILabs Business Source License 1.1 (`LICENSE`).
