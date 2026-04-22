"""End-to-end smoke tests for TriPy."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

import tripy

ROOT = Path(__file__).resolve().parents[2]


def _trinary_bin() -> Path:
    exe = ROOT / "build" / "bin" / "trinary.exe"
    if exe.exists():
        return exe
    return ROOT / "build" / "bin" / "trinary"


def test_version_string() -> None:
    v = tripy.version()
    assert "trinary" in v
    assert "0.1.0" in v


def test_features_dict_has_known_keys() -> None:
    f = tripy.features()
    for k in ("sse2", "avx", "avx2", "avx512f", "bmi2", "popcnt", "fma"):
        assert k in f
        assert isinstance(f[k], bool)


def test_braincore_small_run() -> None:
    r = tripy.braincore(neurons=4096, iterations=10)
    assert r["kernel"] == "braincore"
    assert r["neurons"] == 4096
    assert r["iterations"] == 10
    assert r["seconds"] > 0
    assert r["giga_neurons_per_sec"] > 0


def test_harding_gate_small_run() -> None:
    r = tripy.harding_gate(count=4096, iterations=4)
    assert r["kernel"] == "harding_gate_i16"
    assert r["elements"] == 4096
    assert r["iterations"] == 4
    assert r["seconds"] > 0


def test_lattice_flip_small_run() -> None:
    r = tripy.lattice_flip(bits=65536, iterations=100)
    assert r["kernel"] == "lattice_flip"
    assert r["iterations"] == 100
    assert r["seconds"] > 0


def test_run_tri_hello(tmp_path: Path) -> None:
    p = tmp_path / "hello.tri"
    p.write_text('print("hi")\n', encoding="utf-8")
    rc = tripy.run_tri(str(p))
    assert rc == 0


def test_run_tri_bad_syntax(tmp_path: Path) -> None:
    p = tmp_path / "bad.tri"
    p.write_text("%% not valid %%\n", encoding="utf-8")
    rc = tripy.run_tri(str(p))
    assert rc != 0


def test_accelerate_dispatches_to_kernel() -> None:
    @tripy.accelerate
    def my_braincore() -> dict:  # type: ignore[type-arg]
        return tripy.braincore(neurons=4096, iterations=5)

    r = my_braincore()
    assert r["kernel"] == "braincore"


def test_accelerate_unknown_body_is_passthrough() -> None:
    @tripy.accelerate
    def plain_python(x: int) -> int:
        return x * 2

    assert plain_python(21) == 42


def test_interference_search_alias() -> None:
    assert tripy.interference_search(100, 42) == 42
    assert tripy.interference_search(100, 100) == -1


def test_engine_facade_search_lattice() -> None:
    assert tripy.engine.search_lattice(100, 77) == 77
    assert tripy.engine.search_lattice(100, -1) == -1


def test_cli_version() -> None:
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", "--version"],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert "tripy 0.1.0" in rc.stdout


def test_cli_braincore_emits_json() -> None:
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", "braincore", "4096", "5"],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    data = json.loads(rc.stdout)
    assert data["kernel"] == "braincore"
    assert data["neurons"] == 4096


def test_cli_runs_tri_example() -> None:
    example = ROOT / "language" / "examples" / "hello.tri"
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(example)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert "hello from trinary" in rc.stdout


def test_cli_runs_py_file(tmp_path: Path) -> None:
    p = tmp_path / "demo.py"
    p.write_text('print("hello from python via tripy")\n', encoding="utf-8")
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(p)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert "hello from python via tripy" in rc.stdout


def test_cli_runs_prime_tri_example() -> None:
    example = ROOT / "language" / "examples" / "prime.tri"
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(example)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert "[trinary] prime:" in rc.stdout


def test_cli_runs_multiple_kernel_statements(tmp_path: Path) -> None:
    p = tmp_path / "multi_prime.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(p)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert rc.stdout.count("[trinary] prime:") == 2


def test_cli_flushes_kernel_queue_before_print(tmp_path: Path) -> None:
    p = tmp_path / "kernel_print_kernel.tri"
    p.write_text('prime(100)\nprint("marker")\nprime(100)\n', encoding="utf-8")
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(p)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    lines = [line.strip() for line in rc.stdout.splitlines() if line.strip()]
    marker_idx = lines.index("marker")
    assert any("[trinary] prime:" in line for line in lines[:marker_idx])
    assert any("[trinary] prime:" in line for line in lines[marker_idx + 1 :])


def test_cli_experimental_opt_can_dedupe_adjacent_prime(tmp_path: Path) -> None:
    p = tmp_path / "multi_prime_optin.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    env = dict(os.environ)
    env["TRINARY_OPT_EXPERIMENTAL"] = "1"
    env["TRINARY_OPT_ALLOW_OBSERVABLE"] = "1"
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(p)],
        capture_output=True,
        text=True,
        check=False,
        env=env,
    )
    assert rc.returncode == 0
    assert rc.stdout.count("[trinary] prime:") == 1


def test_cli_opt_trace_emits_telemetry(tmp_path: Path) -> None:
    p = tmp_path / "trace_prime.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    env = dict(os.environ)
    env["TRINARY_OPT_TRACE"] = "1"
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", str(p)],
        capture_output=True,
        text=True,
        check=False,
        env=env,
    )
    assert rc.returncode == 0
    assert "[tri][opt] seen=" in rc.stderr
    assert "[tri][opt-total] seen=" in rc.stderr


def test_cli_flags_enable_experimental_optimizer(tmp_path: Path) -> None:
    p = tmp_path / "multi_prime_flags.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    rc = subprocess.run(
        [
            sys.executable,
            "-m",
            "tripy.cli",
            "--opt-experimental",
            "--opt-allow-observable",
            str(p),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert rc.stdout.count("[trinary] prime:") == 1


def test_cli_flags_enable_opt_trace(tmp_path: Path) -> None:
    p = tmp_path / "trace_prime_flags.tri"
    p.write_text("prime(100)\n", encoding="utf-8")
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", "--opt-trace", str(p)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert "[tri][opt] seen=" in rc.stderr


def test_cli_opt_allow_observable_alone_does_not_transform(tmp_path: Path) -> None:
    p = tmp_path / "multi_prime_obs_only.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", "--opt-allow-observable", str(p)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert rc.stdout.count("[trinary] prime:") == 2


def test_cli_opt_default_overrides_inherited_env(tmp_path: Path) -> None:
    p = tmp_path / "multi_prime_default_override.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    env = dict(os.environ)
    env["TRINARY_OPT_EXPERIMENTAL"] = "1"
    env["TRINARY_OPT_ALLOW_OBSERVABLE"] = "1"
    env["TRINARY_OPT_TRACE"] = "1"
    rc = subprocess.run(
        [sys.executable, "-m", "tripy.cli", "--opt-default", str(p)],
        capture_output=True,
        text=True,
        check=False,
        env=env,
    )
    assert rc.returncode == 0
    assert rc.stdout.count("[trinary] prime:") == 2
    assert "[tri][opt] seen=" not in rc.stderr


def test_native_cli_opt_trace_flag(tmp_path: Path) -> None:
    p = tmp_path / "native_trace.tri"
    p.write_text("prime(100)\n", encoding="utf-8")
    rc = subprocess.run(
        [str(_trinary_bin()), "--opt-trace", str(p)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert "[tri][opt] seen=" in rc.stderr


def test_native_cli_opt_experimental_flags(tmp_path: Path) -> None:
    p = tmp_path / "native_optin.tri"
    p.write_text("prime(100)\nprime(100)\n", encoding="utf-8")
    rc = subprocess.run(
        [
            str(_trinary_bin()),
            "--opt-experimental",
            "--opt-allow-observable",
            str(p),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert rc.returncode == 0
    assert rc.stdout.count("[trinary] prime:") == 1
