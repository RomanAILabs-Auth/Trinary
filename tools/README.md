# tools/

Bundled developer-facing tools. One subdirectory per tool.

| Subdir        | Purpose                                      | Status           |
|---------------|----------------------------------------------|------------------|
| `zig/`        | Bundled Zig 0.16.0 (Clang + MinGW-w64 + lld) | **Shipped** (D-008) |
| `tri-fmt/`    | `.tri` source formatter                      | Reserved (Phase 2) |
| `tri-lsp/`    | `.tri` Language Server Protocol server       | Reserved (Phase 2) |
| `tri-bench/`  | Interactive benchmark runner + plot emitter  | Reserved (Phase 1+)|

## zig/

The only tool that currently has content. `build.ps1` invokes
`tools/zig/zig.exe` directly. It is not a submodule, not vendored via a
package manager — it is a specific pinned binary for reproducibility. To
refresh, see `scripts/install_zig.ps1` (and log a decision row in the
architecture MD §8).

## tri-fmt / tri-lsp / tri-bench

These directories exist to satisfy §2 of the architecture doc. They are
empty until the respective phase opens. Adding code here without a parallel
quality gate (lint + test + CI) would create maintenance debt.

Each tool, when opened, will follow the same pattern as the engine:
- typed public surface
- unit tests with ≥90 % coverage gate
- CI job that exercises it on every PR
- documentation generated from code
