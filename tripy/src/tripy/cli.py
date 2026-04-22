"""TriPy CLI.

Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com

Contract (frozen in architecture §1.2 / §4):
    tripy <file.py>      Execute Python; @accelerate routes to machine code.
    tripy <file.tri>     Compile & run the .tri file directly.
    tripy braincore      Canonical neuromorphic benchmark.
    tripy bench          Full benchmark suite (JSON).
    tripy repl           Interactive REPL.
    tripy --version
    tripy --help
"""

from __future__ import annotations

import json
import os
import runpy
import sys
from collections.abc import Sequence

from . import __version__, runtime

USAGE = """\
tripy — Python front-end for the Trinary machine-code engine

Usage:
  tripy [opt flags] <file.py>   Run Python file; @accelerate routes to kernels.
  tripy [opt flags] <file.tri>  Compile and run a .tri program.
  tripy [opt flags] braincore [N] [ITER]    Canonical benchmark.
  tripy [opt flags] bench       Full benchmark suite → JSON.
  tripy [opt flags] repl        Interactive REPL (Python + .tri).
  tripy [opt flags] --version   Engine + package version.
  tripy [opt flags] --features  Detected CPU features.
  tripy --help                  Show this message.

Optimizer flags (all optional, default-safe):
  --opt-default                 Disable optimizer toggles.
  --opt-experimental            Enable experimental optimizer transforms.
  --opt-allow-observable        Allow transforms that can alter output.
  --opt-trace                   Emit optimizer telemetry to stderr.

Legacy direct forms (still valid):
  tripy <file.py>               Run a Python file; @accelerate routes to kernels.
  tripy <file.tri>              Compile and run a .tri program.
  tripy braincore [N] [ITER]    Canonical 8-bit neuromorphic benchmark.
  tripy bench                   Full benchmark suite → JSON.
  tripy repl                    Interactive REPL (Python + .tri).
  tripy --version               Engine + package version.
  tripy --features              Detected CPU features.
  tripy --help                  Show this message.
"""


def _cmd_braincore(argv: Sequence[str]) -> int:
    neurons = 4_000_000
    iters = 1000
    threshold = 200
    positional: list[str] = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--neurons" and i + 1 < len(argv):
            neurons = int(argv[i + 1])
            i += 2
            continue
        if arg == "--iters" and i + 1 < len(argv):
            iters = int(argv[i + 1])
            i += 2
            continue
        if arg == "--threshold" and i + 1 < len(argv):
            threshold = int(argv[i + 1])
            i += 2
            continue
        positional.append(arg)
        i += 1
    if len(positional) > 0:
        neurons = int(positional[0])
    if len(positional) > 1:
        iters = int(positional[1])
    result = runtime.braincore(
        neurons=neurons,
        iterations=iters,
        threshold=threshold,
    )
    print(json.dumps(result, indent=2))
    return 0


def _cmd_bench(_argv: Sequence[str]) -> int:
    results = [
        runtime.braincore(),
        runtime.harding_gate(),
        runtime.lattice_flip(),
    ]
    print(json.dumps(results, indent=2))
    return 0


def _cmd_repl(_argv: Sequence[str]) -> int:
    import code

    banner = (
        f"TriPy {__version__} — {runtime.version()}\n"
        "Type `.tri <src>` to evaluate a .tri snippet.  Ctrl-D / Ctrl-Z to exit."
    )
    ns = {"tripy": sys.modules[__package__], "runtime": runtime}
    console = code.InteractiveConsole(locals=ns)
    console.interact(banner=banner, exitmsg="")
    return 0


def _run_py(path: str) -> int:
    if not os.path.exists(path):
        print(f"error: file not found: {path}", file=sys.stderr)
        return 2
    runpy.run_path(path, run_name="__main__")
    return 0


def _run_tri(path: str) -> int:
    if not os.path.exists(path):
        print(f"error: file not found: {path}", file=sys.stderr)
        return 2
    rc = runtime.run_tri(path)
    return 0 if rc == 0 else 2


def _extract_runtime_opt_flags(args: list[str]) -> tuple[list[str], dict[str, str]]:
    updates: dict[str, str] = {}
    rest: list[str] = []
    for a in args:
        if a == "--opt-default":
            updates["TRINARY_OPT_EXPERIMENTAL"] = "0"
            updates["TRINARY_OPT_ALLOW_OBSERVABLE"] = "0"
            updates["TRINARY_OPT_TRACE"] = "0"
            continue
        if a == "--opt-experimental":
            updates["TRINARY_OPT_EXPERIMENTAL"] = "1"
            continue
        if a == "--opt-allow-observable":
            updates["TRINARY_OPT_ALLOW_OBSERVABLE"] = "1"
            continue
        if a == "--opt-trace":
            updates["TRINARY_OPT_TRACE"] = "1"
            continue
        rest.append(a)
    return rest, updates


def main(argv: Sequence[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    args, updates = _extract_runtime_opt_flags(args)
    env_keys = ("TRINARY_OPT_EXPERIMENTAL", "TRINARY_OPT_ALLOW_OBSERVABLE", "TRINARY_OPT_TRACE")
    prev: dict[str, str | None] = {k: os.environ.get(k) for k in env_keys}
    for key, value in updates.items():
        os.environ[key] = value

    try:
        if not args or args[0] in ("-h", "--help"):
            print(USAGE, end="")
            return 0
        head = args[0]
        if head == "--version":
            print(f"tripy {__version__}")
            print(runtime.version())
            return 0
        if head == "--features":
            print(json.dumps(runtime.features(), indent=2))
            return 0
        if head == "braincore":
            return _cmd_braincore(args[1:])
        if head == "bench":
            return _cmd_bench(args[1:])
        if head == "repl":
            return _cmd_repl(args[1:])
        if head.endswith(".py"):
            return _run_py(head)
        if head.endswith(".tri"):
            return _run_tri(head)
        print(f"error: unknown argument: {head}\n\n{USAGE}", file=sys.stderr)
        return 2
    finally:
        for key, prev_value in prev.items():
            if prev_value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = prev_value


if __name__ == "__main__":
    sys.exit(main())
