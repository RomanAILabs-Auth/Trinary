cmake_minimum_required(VERSION 3.23)
project(trinary_engine LANGUAGES C ASM)

# -----------------------------------------------------------------------------
# Stub CMake file (intentionally non-authoritative).
#
# Authoritative build paths remain:
#   - build.ps1 (Windows, bundled Zig toolchain)
#   - build.sh  (POSIX, system cc / optional zig cc)
#
# See architecture decisions D-008, D-009, D-010 in
# trinary-tripy-architecture.md. We keep this file so §2 layout is structurally
# complete and to provide a future integration point for scikit-build-core /
# cibuildwheel in Phase 3.
# -----------------------------------------------------------------------------

message(STATUS "Trinary engine CMake stub loaded.")
message(STATUS "Use build.ps1 or build.sh for production builds.")

