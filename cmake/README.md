# cmake/

**Status:** reserved. **No CMake files live here yet.**

## Why this directory is empty

`§2` of `trinary-tripy-architecture.md` describes a CMake-orchestrated build.
That description is **superseded by decision D-008**:

> Build toolchain: bundled Zig 0.16.0 (`tools/zig/zig.exe`) — Clang 21 +
> MinGW-w64 libc + lld. Author's Windows has MSVC 2022 but no Windows SDK.
> Zig bundles libc headers + linker; single-binary, cross-platform,
> reproducible. No MSVC / no SDK dependency.

Today's authoritative build entry points are:

- `build.ps1` — Windows (uses `tools/zig/zig.exe`).
- `build.sh`  — POSIX (uses system `cc`; set `CC="zig cc"` for parity).

## What will land here (Phase 3+ wheel work)

When `pip install tripy` gains wheel builds via `cibuildwheel`, this directory
will host:

- `toolchain.cmake`         — maps CMake's compiler/linker vars to `zig cc`.
- `detect_arch.cmake`       — x86_64 vs aarch64 dispatch.
- `trinary-config.cmake.in` — installed CMake package config for downstream
  C/C++ consumers linking `libtrinary`.

Until then, adding CMake files here without a corresponding build gate would
create a second source of truth and divergence risk.
