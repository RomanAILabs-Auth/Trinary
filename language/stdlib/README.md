# language/stdlib/

**Status:** reserved. Phase 2 — **no stdlib modules ship with the v0 interpreter.**

## What will live here

`.tri` source modules providing the built-in symbols the language spec
(`language/spec/grammar.ebnf`) will eventually call out. Planned modules:

| Module        | Purpose                                                      |
|---------------|--------------------------------------------------------------|
| `braincore.tri` | LIF neuron helpers, threshold curves, spike-train generators |
| `lattice.tri`   | Bit-packed trit lattices, mask helpers, topology queries     |
| `harding.tri`   | Harding-gate convenience wrappers                            |
| `math.tri`      | Trit arithmetic, balanced-ternary ops, saturating math       |
| `io.tri`        | `print`, `read`, file IO (thin shims over engine syscalls)   |
| `prelude.tri`   | Auto-imported; re-exports the minimum viable surface         |

## Contract

1. Every stdlib module is pure `.tri`. No C, no Python.
2. Every exported function has a matching test under
   `language/tests/stdlib/<module>_test.tri` and a doctest in its docstring.
3. Version lock-step with the language: `.tri` stdlib ships with the engine
   that understands it, not independently. Breaking changes bump the engine's
   minor version.

## Why the directory exists now if it's empty

§2 of the architecture doc mandates this path. Creating the empty directory
with this README makes the compliance matrix (§2.1) green on structure while
keeping the deferral honest.
