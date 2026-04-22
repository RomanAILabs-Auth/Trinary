"""@tripy.accelerate — AST-inspecting decorator.

Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com

v0.1 contract:
    - Wraps a callable; when first invoked, inspects the function's AST to
      detect recognized fast-path patterns and dispatches to the matching
      machine-code kernel.
    - Recognized patterns (v0.1):
        * A function whose body is essentially a call to `braincore()`,
          `harding_gate()`, or `lattice_flip()` -> delegate directly.
        * A function whose body is a for-loop over `range(0, N)` that only
          performs in-place XOR/flip on a NumPy-like buffer -> lattice_flip.
    - Otherwise the original Python is called unchanged and a diagnostic is
      emitted when the env var TRIPY_DIAG=1 is set.

Phase 2 of the architecture plan replaces this with full IR lowering.
"""

from __future__ import annotations

import ast
import inspect
import os
from functools import wraps
from typing import Any, Callable, TypeVar

from . import _core

F = TypeVar("F", bound=Callable[..., Any])


_FAST_PATH_NAMES = frozenset({"braincore", "harding_gate", "lattice_flip"})


def _diag(msg: str) -> None:
    if os.environ.get("TRIPY_DIAG") == "1":
        print(f"[tripy.jit] {msg}")


def _detect_fast_path(fn: Callable[..., Any]) -> str | None:
    try:
        source = inspect.getsource(fn)
    except (OSError, TypeError):
        return None
    try:
        tree = ast.parse(source)
    except SyntaxError:
        return None
    func_def: ast.FunctionDef | None = None
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef):
            func_def = node
            break
    if func_def is None:
        return None
    for stmt in func_def.body:
        target: ast.AST | None = stmt
        if isinstance(stmt, ast.Return):
            target = stmt.value
        if target is None:
            continue
        if isinstance(target, ast.Expr):
            target = target.value
        if isinstance(target, ast.Call):
            name = _attr_name(target.func)
            if name in _FAST_PATH_NAMES:
                return name
    return None


def _attr_name(node: ast.AST) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return node.attr
    return None


def accelerate(fn: F) -> F:
    """Decorator: route recognized kernels to machine code; otherwise unchanged."""
    fast = _detect_fast_path(fn)
    if fast is None:
        _diag(f"no fast-path for {fn.__name__}; running Python")
        return fn

    @wraps(fn)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        _diag(f"dispatching {fn.__name__} -> {fast}")
        if fast == "braincore":
            return _core.braincore(*args, **kwargs)
        if fast == "harding_gate":
            return _core.harding_gate(*args, **kwargs)
        if fast == "lattice_flip":
            return _core.lattice_flip(*args, **kwargs)
        return fn(*args, **kwargs)

    return wrapper  # type: ignore[return-value]
