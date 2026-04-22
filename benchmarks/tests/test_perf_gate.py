"""Self-tests for benchmarks/perf_gate.py."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GATE = ROOT / "benchmarks" / "perf_gate.py"


def _make(path: Path, results: list[dict]) -> Path:
    path.write_text(json.dumps({"schema": "trinary.bench.v1", "results": results}))
    return path


def _run(baseline: Path, current: Path, tol: float = 0.03) -> subprocess.CompletedProcess:
    return subprocess.run(
        [sys.executable, str(GATE),
         "--baseline", str(baseline),
         "--current",  str(current),
         "--tolerance", str(tol)],
        capture_output=True, text=True, check=False,
    )


def test_within_tolerance_passes(tmp_path: Path) -> None:
    b = _make(tmp_path / "b.json",
              [{"name": "k", "gelems_per_sec_best": 100.0}])
    c = _make(tmp_path / "c.json",
              [{"name": "k", "gelems_per_sec_best": 98.0}])  # -2% regression
    r = _run(b, c)
    assert r.returncode == 0, r.stderr


def test_regression_fails(tmp_path: Path) -> None:
    b = _make(tmp_path / "b.json",
              [{"name": "k", "gelems_per_sec_best": 100.0}])
    c = _make(tmp_path / "c.json",
              [{"name": "k", "gelems_per_sec_best": 90.0}])  # -10% regression
    r = _run(b, c)
    assert r.returncode == 2, r.stdout + r.stderr


def test_improvement_passes(tmp_path: Path) -> None:
    b = _make(tmp_path / "b.json",
              [{"name": "k", "gelems_per_sec_best": 100.0}])
    c = _make(tmp_path / "c.json",
              [{"name": "k", "gelems_per_sec_best": 200.0}])  # +100%
    r = _run(b, c)
    assert r.returncode == 0, r.stderr
    assert "BETTER" in r.stdout


def test_new_kernel_does_not_fail(tmp_path: Path) -> None:
    b = _make(tmp_path / "b.json", [])
    c = _make(tmp_path / "c.json",
              [{"name": "new_kernel", "gelems_per_sec_best": 1.0}])
    r = _run(b, c)
    assert r.returncode == 0, r.stderr
    assert "NEW" in r.stdout
