"""
Elite local benchmark: Trinary vs major language runtimes.

Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com
"""
from __future__ import annotations

import json
import os
import platform
import re
import shutil
import statistics
import subprocess
import tempfile
import time
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_JSON = ROOT / "benchmarks" / "elite_results.json"
OUT_MD = ROOT / "benchmarks" / "elite_results.md"
LIMIT = 5_000_000
RUNS = 3


@dataclass
class BenchResult:
    name: str
    available: bool
    seconds: float | None
    mnums_per_sec: float | None
    prime_count: int | None
    note: str = ""


def run_cmd(cmd: list[str], *, cwd: Path | None = None) -> tuple[int, str, str]:
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        capture_output=True,
        text=True,
        check=False,
    )
    return proc.returncode, proc.stdout, proc.stderr


def measure_best(fn: Callable[[], tuple[int, float]]) -> tuple[int, float]:
    counts: list[int] = []
    times: list[float] = []
    for _ in range(RUNS):
        c, t = fn()
        counts.append(c)
        times.append(t)
    if len(set(counts)) != 1:
        raise RuntimeError(f"inconsistent prime counts: {counts}")
    return counts[0], min(times)


def bench_trinary() -> BenchResult:
    trinary_exe = ROOT / "build" / "bin" / "trinary.exe"
    if not trinary_exe.exists():
        trinary_exe = ROOT / "build" / "bin" / "trinary"
    if not trinary_exe.exists():
        return BenchResult("Trinary (native)", False, None, None, None, "binary not found")

    def one() -> tuple[int, float]:
        rc, out, err = run_cmd([str(trinary_exe), "prime", str(LIMIT)])
        if rc != 0:
            raise RuntimeError(f"trinary failed: {err.strip()}")
        data = json.loads(out)
        return int(data["prime_count"]), float(data["seconds"])

    count, best = measure_best(one)
    return BenchResult("Trinary (native)", True, best, LIMIT / best / 1e6, count)


def bench_tripy() -> BenchResult:
    with tempfile.TemporaryDirectory() as td:
        tri = Path(td) / "prime.tri"
        tri.write_text(f"prime({LIMIT})\n", encoding="utf-8")

        rgx = re.compile(r"count=(\d+)\s+time=([0-9.]+)")

        def one() -> tuple[int, float]:
            rc, out, err = run_cmd(["python", "-m", "tripy.cli", str(tri)], cwd=ROOT)
            if rc != 0:
                raise RuntimeError(f"tripy failed: {err.strip()}")
            m = rgx.search(out)
            if not m:
                raise RuntimeError(f"unable to parse tripy output: {out}")
            return int(m.group(1)), float(m.group(2))

        count, best = measure_best(one)
        return BenchResult("TriPy (.tri via CLI)", True, best, LIMIT / best / 1e6, count)


def bench_python() -> BenchResult:
    py_code = r"""
import time
N = int(__import__("sys").argv[1])
def pi_n(n: int) -> int:
    if n < 2:
        return 0
    s = bytearray(b"\x01") * (n + 1)
    s[0:2] = b"\x00\x00"
    p = 2
    while p * p <= n:
        if s[p]:
            start = p * p
            step = p
            s[start:n+1:step] = b"\x00" * (((n - start) // step) + 1)
        p += 1
    return sum(s)
t0 = time.perf_counter()
c = pi_n(N)
t1 = time.perf_counter()
print(f"{c} {t1-t0:.9f}")
"""

    def one() -> tuple[int, float]:
        rc, out, err = run_cmd(["python", "-c", py_code, str(LIMIT)])
        if rc != 0:
            raise RuntimeError(f"python failed: {err.strip()}")
        c, t = out.strip().split()
        return int(c), float(t)

    count, best = measure_best(one)
    return BenchResult("Python 3.13 (pure)", True, best, LIMIT / best / 1e6, count)


def bench_node() -> BenchResult:
    if shutil.which("node") is None:
        return BenchResult("Node.js (pure)", False, None, None, None, "node not installed")

    js_code = r"""
const N = Number(process.argv[1]);
function piN(n) {
  if (n < 2) return 0;
  const s = new Uint8Array(n + 1);
  s.fill(1);
  s[0] = 0; s[1] = 0;
  for (let p = 2; p * p <= n; p++) {
    if (!s[p]) continue;
    for (let k = p * p; k <= n; k += p) s[k] = 0;
  }
  let c = 0;
  for (let i = 2; i <= n; i++) c += s[i];
  return c;
}
const t0 = performance.now();
const c = piN(N);
const t1 = performance.now();
console.log(`${c} ${((t1 - t0) / 1000).toFixed(9)}`);
"""

    def one() -> tuple[int, float]:
        rc, out, err = run_cmd(["node", "-e", js_code, str(LIMIT)])
        if rc != 0:
            raise RuntimeError(f"node failed: {err.strip()}")
        c, t = out.strip().split()
        return int(c), float(t)

    count, best = measure_best(one)
    return BenchResult("Node.js 22 (pure)", True, best, LIMIT / best / 1e6, count)


def bench_go() -> BenchResult:
    if shutil.which("go") is None:
        return BenchResult("Go (compiled)", False, None, None, None, "go not installed")

    go_src = r"""
package main
import (
  "fmt"
  "os"
  "strconv"
  "time"
)
func piN(n int) int {
  if n < 2 { return 0 }
  s := make([]byte, n+1)
  for i := range s { s[i] = 1 }
  s[0], s[1] = 0, 0
  for p := 2; p*p <= n; p++ {
    if s[p] == 0 { continue }
    for k := p*p; k <= n; k += p {
      s[k] = 0
    }
  }
  c := 0
  for i := 2; i <= n; i++ { c += int(s[i]) }
  return c
}
func main() {
  n, _ := strconv.Atoi(os.Args[1])
  t0 := time.Now()
  c := piN(n)
  dt := time.Since(t0).Seconds()
  fmt.Printf("%d %.9f\n", c, dt)
}
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        src = td_path / "main.go"
        exe = td_path / ("bench.exe" if os.name == "nt" else "bench")
        src.write_text(go_src, encoding="utf-8")
        rc, _, err = run_cmd(["go", "build", "-o", str(exe), str(src)])
        if rc != 0:
            return BenchResult("Go (compiled)", False, None, None, None, f"build failed: {err.strip()}")

        def one() -> tuple[int, float]:
            rc2, out, err2 = run_cmd([str(exe), str(LIMIT)])
            if rc2 != 0:
                raise RuntimeError(f"go binary failed: {err2.strip()}")
            c, t = out.strip().split()
            return int(c), float(t)

        count, best = measure_best(one)
        return BenchResult("Go 1.26 (compiled)", True, best, LIMIT / best / 1e6, count)


def bench_cpp() -> BenchResult:
    zig = ROOT / "tools" / "zig" / "zig.exe"
    compiler: list[str] | None = None
    if zig.exists():
        compiler = [str(zig), "c++"]
    elif shutil.which("g++"):
        compiler = ["g++"]
    elif shutil.which("clang++"):
        compiler = ["clang++"]

    if compiler is None:
        return BenchResult("C++ (compiled)", False, None, None, None, "no C++ compiler found")

    cpp_src = r"""
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
static int piN(int n) {
  if (n < 2) return 0;
  std::vector<unsigned char> s((size_t)n + 1, 1);
  s[0] = 0; s[1] = 0;
  for (int p = 2; p * p <= n; ++p) {
    if (!s[p]) continue;
    for (int k = p * p; k <= n; k += p) s[k] = 0;
  }
  int c = 0;
  for (int i = 2; i <= n; ++i) c += s[i] ? 1 : 0;
  return c;
}
int main(int argc, char** argv) {
  int n = std::atoi(argv[1]);
  auto t0 = std::chrono::high_resolution_clock::now();
  int c = piN(n);
  auto t1 = std::chrono::high_resolution_clock::now();
  double dt = std::chrono::duration<double>(t1 - t0).count();
  std::printf("%d %.9f\n", c, dt);
  return 0;
}
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        src = td_path / "main.cpp"
        exe = td_path / ("bench.exe" if os.name == "nt" else "bench")
        src.write_text(cpp_src, encoding="utf-8")
        rc, _, err = run_cmd(compiler + ["-O3", "-std=c++17", str(src), "-o", str(exe)])
        if rc != 0:
            return BenchResult("C++ (compiled)", False, None, None, None, f"build failed: {err.strip()}")

        def one() -> tuple[int, float]:
            rc2, out, err2 = run_cmd([str(exe), str(LIMIT)])
            if rc2 != 0:
                raise RuntimeError(f"c++ binary failed: {err2.strip()}")
            c, t = out.strip().split()
            return int(c), float(t)

        count, best = measure_best(one)
        return BenchResult("C++17 -O3 (compiled)", True, best, LIMIT / best / 1e6, count)


def bench_rust() -> BenchResult:
    rustc = shutil.which("rustc")
    if rustc is None:
        return BenchResult("Rust (compiled)", False, None, None, None, "rustc not installed")

    rs_src = r"""
use std::env;
use std::time::Instant;

fn pi_n(n: usize) -> usize {
    if n < 2 { return 0; }
    let mut s = vec![true; n + 1];
    s[0] = false; s[1] = false;
    let mut p = 2usize;
    while p * p <= n {
        if s[p] {
            let mut k = p * p;
            while k <= n {
                s[k] = false;
                k += p;
            }
        }
        p += 1;
    }
    s.iter().skip(2).filter(|&&v| v).count()
}

fn main() {
    let n: usize = env::args().nth(1).unwrap().parse().unwrap();
    let t0 = Instant::now();
    let c = pi_n(n);
    let dt = t0.elapsed().as_secs_f64();
    println!("{} {:.9}", c, dt);
}
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        src = td_path / "main.rs"
        exe = td_path / ("bench.exe" if os.name == "nt" else "bench")
        src.write_text(rs_src, encoding="utf-8")
        rc, _, err = run_cmd([rustc, "-O", str(src), "-o", str(exe)])
        if rc != 0:
            return BenchResult("Rust (compiled)", False, None, None, None, f"build failed: {err.strip()}")

        def one() -> tuple[int, float]:
            rc2, out, err2 = run_cmd([str(exe), str(LIMIT)])
            if rc2 != 0:
                raise RuntimeError(f"rust binary failed: {err2.strip()}")
            c, t = out.strip().split()
            return int(c), float(t)

        count, best = measure_best(one)
        return BenchResult("Rust -O (compiled)", True, best, LIMIT / best / 1e6, count)


def bench_mojo() -> BenchResult:
    mojo = shutil.which("mojo")
    if mojo is None:
        return BenchResult("Mojo", False, None, None, None, "mojo not installed")

    mojo_src = r"""
from sys import argv
from time import perf_counter

def pi_n(n: Int) -> Int:
    if n < 2:
        return 0
    var s = [True] * (n + 1)
    s[0] = False
    s[1] = False
    var p = 2
    while p * p <= n:
        if s[p]:
            var k = p * p
            while k <= n:
                s[k] = False
                k += p
        p += 1
    var c = 0
    for i in range(2, n + 1):
        if s[i]:
            c += 1
    return c

fn main():
    let n = Int(argv()[1])
    let t0 = perf_counter()
    let c = pi_n(n)
    let t1 = perf_counter()
    print(c, t1 - t0)
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        src = td_path / "main.mojo"
        src.write_text(mojo_src, encoding="utf-8")

        def one() -> tuple[int, float]:
            rc, out, err = run_cmd([mojo, "run", str(src), str(LIMIT)])
            if rc != 0:
                raise RuntimeError(f"mojo run failed: {err.strip()}")
            parts = out.strip().split()
            if len(parts) < 2:
                raise RuntimeError(f"unexpected mojo output: {out}")
            return int(parts[0]), float(parts[1])

        try:
            count, best = measure_best(one)
        except Exception as exc:  # noqa: BLE001
            return BenchResult("Mojo", False, None, None, None, str(exc))
        return BenchResult("Mojo (run)", True, best, LIMIT / best / 1e6, count)


def write_results(results: list[BenchResult]) -> None:
    host = {
        "platform": platform.platform(),
        "python": platform.python_version(),
        "limit": LIMIT,
        "runs": RUNS,
    }
    payload = {
        "host": host,
        "results": [r.__dict__ for r in results],
    }
    OUT_JSON.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    lines = [
        "# Elite Benchmark Snapshot",
        "",
        f"- Platform: `{host['platform']}`",
        f"- Prime limit: `{LIMIT:,}`",
        f"- Runs per benchmark: `{RUNS}` (best time reported)",
        "",
        "| Runtime | Available | Best seconds | Throughput (M nums/s) | pi(limit) |",
        "|---|---:|---:|---:|---:|",
    ]
    for r in results:
        if not r.available:
            lines.append(f"| {r.name} | no | - | - | - |")
        else:
            lines.append(
                f"| {r.name} | yes | {r.seconds:.6f} | {r.mnums_per_sec:.2f} | {r.prime_count} |"
            )
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    benches = [
        bench_trinary,
        bench_tripy,
        bench_cpp,
        bench_rust,
        bench_mojo,
        bench_python,
        bench_node,
        bench_go,
    ]
    results: list[BenchResult] = []
    for b in benches:
        try:
            results.append(b())
        except Exception as exc:  # noqa: BLE001
            results.append(BenchResult(b.__name__, False, None, None, None, str(exc)))

    expected = None
    for r in results:
        if r.name == "Trinary (native)" and r.available:
            expected = r.prime_count
            break
    if expected is not None:
        for r in results:
            if r.available and r.prime_count is not None and r.prime_count != expected:
                r.note = f"invalid prime_count (expected {expected})"
                r.available = False
                r.seconds = None
                r.mnums_per_sec = None
                r.prime_count = None
    # Stable, human-useful order: available first, then throughput desc.
    results.sort(key=lambda r: (not r.available, -(r.mnums_per_sec or 0.0), r.name))
    write_results(results)
    print(f"Wrote: {OUT_JSON}")
    print(f"Wrote: {OUT_MD}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
