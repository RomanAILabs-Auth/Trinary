"""TriPy — Python front-end for the Trinary machine-code engine.

Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com

Run arbitrary Python with accelerated kernels, or execute `.tri` programs
directly through the compiler.

Public API:
    tripy.braincore(neurons, iterations) -> dict
    tripy.harding_gate(count, iterations) -> dict
    tripy.lattice_flip(bits, iterations) -> dict
    tripy.run_tri(path) -> int
    tripy.features() -> dict[str, bool]
    tripy.version() -> str
    @tripy.accelerate
"""

from __future__ import annotations

from . import _core
from .jit import accelerate
from .runtime import (
    active_variant,
    braincore,
    features,
    harding_gate,
    lattice_flip,
    run_tri,
    search_lattice,
    version,
)


def interference_search(space_size: int, target: int) -> int:
    """Compatibility alias for older search demo scripts."""
    return search_lattice(space_size, target)


class _EngineFacade:
    """Small compatibility surface for scripts using `tripy.engine.*`."""

    @staticmethod
    def search_lattice(space_size: int, target: int) -> int:
        return search_lattice(space_size, target)


engine = _EngineFacade()

__all__ = [
    "_core",
    "accelerate",
    "active_variant",
    "braincore",
    "engine",
    "features",
    "harding_gate",
    "interference_search",
    "lattice_flip",
    "run_tri",
    "search_lattice",
    "version",
]

__version__ = "0.1.0"
