"""Thin typed wrappers over the C extension.

Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com
"""

from __future__ import annotations

from typing import TypedDict, cast

from . import _core


class BraincoreResult(TypedDict):
    kernel: str
    variant: str
    neurons: int
    iterations: int
    seconds: float
    giga_neurons_per_sec: float


class HardingResult(TypedDict):
    kernel: str
    variant: str
    elements: int
    iterations: int
    seconds: float
    giga_elements_per_sec: float


class LatticeFlipResult(TypedDict):
    kernel: str
    variant: str
    bits: int
    iterations: int
    seconds: float
    giga_bits_per_sec: float


def version() -> str:
    """Return the engine version banner."""
    return _core.version()


def features() -> dict[str, bool]:
    """Return a dict of detected CPU feature flags."""
    return _core.features()


def active_variant(kernel: str) -> str:
    """Return the best-available implementation name for a kernel."""
    return _core.active_variant(kernel)


def braincore(
    neurons: int = 4_000_000, iterations: int = 1000, threshold: int = 200
) -> BraincoreResult:
    """Run the 8-bit LIF neuromorphic lattice kernel."""
    return cast(BraincoreResult, _core.braincore(neurons, iterations, threshold))


def harding_gate(count: int = 16 * 1024 * 1024, iterations: int = 64) -> HardingResult:
    """Run the Harding-Gate int16 kernel: out = (a*b) - (a^b)."""
    return cast(HardingResult, _core.harding_gate(count, iterations))


def lattice_flip(bits: int = 1 << 24, iterations: int = 1000) -> LatticeFlipResult:
    """Run the bit-packed lattice flip kernel."""
    return cast(LatticeFlipResult, _core.lattice_flip(bits, iterations))


def run_tri(path: str) -> int:
    """Execute a .tri file through the Trinary compiler. Returns status (0 = OK)."""
    return _core.run_tri(path)


def search_lattice(space_size: int, target: int) -> int:
    """Search index-space [0, space_size) for target.

    This is a utility shim for compatibility with older scripts that expect a
    lattice-search style API. It is not a dedicated Trinary kernel path.
    """
    if space_size < 0:
        raise ValueError("space_size must be >= 0")
    if target < 0 or target >= space_size:
        return -1
    return target
