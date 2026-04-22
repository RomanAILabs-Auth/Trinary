# Trinary Engine

Trinary is the machine-code-first engine inside the dual repo.

## Scope

- kernel execution (`braincore`, `harding`, `lattice`, `rotor`, `prime`)
- CPU feature dispatch
- allocator/runtime glue
- `.tri` front-end pipeline (`lexer -> parser -> sema -> IR -> optimizer -> execute`)
- stable C ABI (`trinary_v1_*`)

## How To Work In Engine (Human + LLM)

- Treat `engine/include/trinary/trinary.h` as a contract.
- Keep SIMD/ASM parity with scalar references.
- Keep optimizer defaults conservative and explicit opt-in for behavior-changing transforms.
- Prefer measurable changes: profile, benchmark, gate, then merge.
- Add tests for every parser/optimizer/kernel behavior update.

## Entry Points

- Native CLI: `engine/src/main.c`
- Parser/runtime execution path: `engine/compiler/parser.c`
- Optimizer: `engine/compiler/optimizer.c`
- IR layer: `engine/compiler/ir.c`
- Kernel tests: `engine/tests/test_kernels.c`

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
