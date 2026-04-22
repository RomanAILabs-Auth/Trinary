# Contributing

## Core rules

1. **Trinary kernels are machine code.** If you touch `engine/asm/`, you're writing
   assembly. C/intrinsics are only acceptable as the reference scalar fallback.
2. **The architecture doc is the contract.** Any change to public API, CLI, ABI,
   or performance gates must first land as an entry in
   `trinary-tripy-architecture.md` (§7 Decision Log), then as code.
3. **Performance gate is sacred.** PRs that regress any kernel's mean latency by
   more than 3% over 10 runs on the reference CI host are rejected.

## Dev loop

```powershell
./build.ps1              # full build (engine + TriPy extension)
./build.ps1 -Tests       # build + run all tests
./scripts/run_tests.ps1  # run tests against an existing build
```

## Code style

- C: clang-format (LLVM style, 4-space indent). clang-tidy must be warning-free.
- ASM: 4-space indent, one instruction per line, ALL CAPS mnemonics in MASM files.
  Every kernel begins with an ABI comment block.
- Python: ruff format + ruff (ALL rules, narrow ignores). mypy --strict clean.

## Commits

Conventional Commits: `feat:`, `fix:`, `perf:`, `refactor:`, `test:`, `docs:`, `build:`.

## Branches

- `main` — always green; protected.
- `feat/<name>` — feature work.
- `fix/<name>` — bug work.

## Reviews

Every PR needs at least one approval and green CI.
