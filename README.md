# Trinary

Copyright (c) RomanAILabs - Daniel Harding (GitHub: RomanAILabs-Auth)  
Honored collaborators: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor  
Contact: `daniel@romanailabs.com` | `romanailabs@gmail.com` | `romanailabs.com`

Pure machine-code compute engine + Python/.tri front-end, built for maximum throughput under strict quality gates.

Trinary is a dual-stack repository:
- **Trinary Engine**: hand-written x86-64/AArch64 assembly kernels, C ABI, load-time dispatch.
- **TriPy**: Python front-end and `.tri` execution path using the same engine and IR pipeline.

The architecture contract is maintained in [`trinary-tripy-architecture.md`](./trinary-tripy-architecture.md) and is treated as source of truth.

---

## What Makes Trinary Different

- **Hot path is assembly-first**: no LLVM dependency in the core execution model, no libc in hot loops.
- **One backend, two developer paths**: `tripy file.py` and `tripy file.tri` converge on the same machine-code engine.
- **Kernel set is explicit and performance-driven**:
  - `braincore_u8` neuromorphic lattice
  - `harding_gate_i16`
  - bitpacked lattice flip
  - Clifford rotor (`rotor_cl4`)
  - prime sieve
- **Dispatch is resolved at init**: no feature branching inside hot loops.
- **Performance is gated**: regressions above 3% fail the perf gate.
- **Release discipline exists in-repo**: install scripts, release checks, benchmark harness, changelog, security and contribution policy.

---

## Performance Snapshot

### Engine Baseline (Architecture Reference Numbers)

`build/bin/trinary-bench` (warmup + timed replicates, release).

| Kernel | Variant | Workload | Best seconds | Throughput |
|---|---|---|---:|---:|
| `braincore_u8` | `avx2` | 4 MiB x 2 buffers, 1000 iters | 0.593 | **6.75 GNeurons/s** |
| `harding_gate_i16` | `avx2` | 96 MiB, 64 iters | 0.451 | **2.38 GElements/s** |
| `lattice_flip` | `avx2` | 64 MiB bitpacked lattice, 64 iters | 0.504 | **68.2 Gbit/s** |
| `prime_sieve` | `scalar` | limit=20,000,000, 8 iters | 0.118 | **1350 MLimits/s** |

### Cross-Language Prime Benchmark (Local Host)

Prime limit: `5,000,000`, best-of-3, validated by matching `pi(n)` result.

| Runtime | Best seconds | Throughput (M nums/s) | `pi(5,000,000)` |
|---|---:|---:|---:|
| TriPy (`.tri` via CLI) | 0.003334 | **1499.70** | 348513 |
| Trinary (native) | 0.003692 | **1354.28** | 348513 |
| Go 1.26 (compiled) | 0.028813 | 173.53 | 348513 |
| C++17 -O3 (compiled) | 0.030871 | 161.97 | 348513 |
| Node.js 22 (pure) | 0.039817 | 125.58 | 348513 |
| Python 3.13 (pure) | 0.057928 | 86.31 | 348513 |

Raw outputs and harness:
- `benchmarks/elite_results.md`
- `benchmarks/elite_results.json`
- `benchmarks/elite_lang_bench.py`

Note: Mojo and Rust lanes are supported in the harness but omitted from the table when toolchains are unavailable on the current host.

---

## Quick Start

### Windows (PowerShell)

```powershell
./build.ps1 -Config Release
./build/bin/trinary.exe --features
python -m tripy.cli --version
python -m tripy.cli "language/examples/prime.tri"
```

Install:

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

Install:

```bash
./scripts/install_trinary.sh
./scripts/install_tripy.sh
```

---

## Project Status

- **Phase 0**: complete
- **Phase 1**: in progress
- Engine and TriPy are live, tested, benchmarked, and release-checked in-repo.

For detailed status and decisions:
- [`trinary-tripy-architecture.md`](./trinary-tripy-architecture.md)
- [`CHANGELOG.md`](./CHANGELOG.md)

---

## Engineering Standards

Trinary is quality-first by design:
- kernel behavior must remain reference-correct
- performance changes must be measured and reproducible
- optimizer behavior-changing transforms remain explicit opt-in
- release checks are scriptable and repeatable

Run full verification:

- Windows: `./scripts/release_check.ps1 -RunTests`
- POSIX: `./scripts/release_check.sh 1`

---

## License

This repository is licensed under the **RomanAILabs Business Source License 1.1 (RBSL 1.1)**.

- Non-commercial research, education, and evaluation are allowed under license terms.
- Commercial use requires a separate written commercial license unless covered by the Additional Use Grant in `LICENSE`.
- Version-level conversion to **Apache-2.0** occurs after the defined 3-year Change Date (or earlier at RomanAILabs discretion).

Commercial licensing and enterprise contact:
- `daniel@romanailabs.com`
- `romanailabs@gmail.com`
- `romanailabs.com`

---

## Honored Collaborators

RomanAILabs honors collaboration with:
- Grok / xAI
- Gemini-Flash / Google
- ChatGPT-5.4 / OpenAI
- Cursor

Contributions are welcome under repository quality gates:
- See [`CONTRIBUTING.md`](./CONTRIBUTING.md)
- See [`SECURITY.md`](./SECURITY.md)
