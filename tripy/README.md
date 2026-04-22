# TriPy

TriPy is the Python front-end for the Trinary machine-code engine.

## CLI

```text
tripy <file.py>
tripy <file.tri>
tripy braincore [N] [ITER]
tripy bench
tripy repl
tripy --version
tripy --features
```

### Optimizer Runtime Flags

```text
--opt-default
--opt-experimental
--opt-allow-observable
--opt-trace
```

These flags are default-safe and scoped to the running `tripy` process.

## Performance Snapshot

From the local elite benchmark (`limit=5,000,000`, best-of-3):
- TriPy `.tri` CLI: `~1499.70 M nums/s`
- Trinary native: `~1354.28 M nums/s`
- Python pure baseline: `~86.31 M nums/s`

See `../benchmarks/elite_results.md` for the full cross-language table.

## Developer Guide (Human + LLM)

- Keep API behavior deterministic and explicit.
- Use `runtime.py` as the primary user-facing kernel wrapper layer.
- Use `cli.py` for command surface consistency and flag behavior.
- Preserve typing quality (`_core.pyi`, `py.typed`) for editor and agent tooling.
- Keep experimental optimizer behavior opt-in only.

When touching TriPy:
1. Update tests in `tripy/tests/test_smoke.py`.
2. Validate with `./build.ps1 -Config Release -Tests` or `./build.sh Release 1`.
3. Ensure default outputs remain unchanged unless explicitly intended.

## Collaborators and Attribution

RomanAILabs honors collaboration with:
- Grok / xAI
- Gemini-Flash / Google
- ChatGPT-5.4 / OpenAI
- Cursor

Primary author:
- Daniel Harding (RomanAILabs)
- `daniel@romanailabs.com`
- `romanailabs@gmail.com`
- `romanailabs.com`

See `../trinary-tripy-architecture.md` for the architecture contract.
