"""Trinary performance-gate diff tool.

Compares a fresh benchmark JSON (from `trinary-bench`) against a baseline
JSON and fails (exit 2) if any kernel's best throughput regresses by more
than the configured tolerance. Decision D-007: default tolerance is 3 %.

Usage:
    python benchmarks/perf_gate.py \
        --baseline benchmarks/baseline.json \
        --current  build/bench/latest.json \
        [--tolerance 0.03]

Exit codes:
    0   all kernels within tolerance (or improved).
    1   usage / parse error.
    2   one or more kernels regressed beyond tolerance.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def _index_results(doc: dict) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for r in doc.get("results", []):
        out[r["name"]] = r
    return out


def _throughput_key(result: dict) -> str:
    for k in result:
        if k.endswith("_per_sec_best"):
            return k
    raise KeyError(f"no throughput key in {result}")


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--baseline", required=True, type=Path)
    ap.add_argument("--current", required=True, type=Path)
    ap.add_argument("--tolerance", type=float, default=0.03)
    args = ap.parse_args(argv)

    if not args.baseline.exists():
        print(f"[perf-gate] baseline not found: {args.baseline}", file=sys.stderr)
        return 1
    if not args.current.exists():
        print(f"[perf-gate] current not found: {args.current}", file=sys.stderr)
        return 1

    base = _index_results(json.loads(args.baseline.read_text()))
    curr = _index_results(json.loads(args.current.read_text()))

    regressed: list[str] = []
    improved: list[str] = []

    print(f"[perf-gate] tolerance = {args.tolerance:.1%}")
    for name, cur in curr.items():
        if name not in base:
            print(f"  {name:20s}  NEW   (no baseline)")
            continue
        bkey = _throughput_key(base[name])
        ckey = _throughput_key(cur)
        if bkey != ckey:
            print(f"  {name:20s}  SKIP  (unit mismatch {bkey} vs {ckey})")
            continue
        b = float(base[name][bkey])
        c = float(cur[ckey])
        delta = (c - b) / b if b else 0.0
        flag = "OK   "
        if delta < -args.tolerance:
            flag = "FAIL "
            regressed.append(name)
        elif delta > args.tolerance:
            flag = "BETTER"
            improved.append(name)
        print(
            f"  {name:20s}  {flag}  baseline={b:10.3f}  "
            f"current={c:10.3f}  delta={delta:+.2%}"
        )

    if regressed:
        print(
            f"[perf-gate] REGRESSION in {len(regressed)} kernel(s): "
            + ", ".join(regressed),
            file=sys.stderr,
        )
        return 2
    print("[perf-gate] all within tolerance.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
