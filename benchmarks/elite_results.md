# Elite Benchmark Snapshot

- Platform: `Windows-11-10.0.26200-SP0`
- Prime limit: `5,000,000`
- Runs per benchmark: `3` (best time reported)

| Runtime | Available | Best seconds | Throughput (M nums/s) | pi(limit) |
|---|---:|---:|---:|---:|
| TriPy (.tri via CLI) | yes | 0.003334 | 1499.70 | 348513 |
| Trinary (native) | yes | 0.003692 | 1354.28 | 348513 |
| Go 1.26 (compiled) | yes | 0.028813 | 173.53 | 348513 |
| C++17 -O3 (compiled) | yes | 0.030871 | 161.97 | 348513 |
| Node.js 22 (pure) | yes | 0.039817 | 125.58 | 348513 |
| Python 3.13 (pure) | yes | 0.057928 | 86.31 | 348513 |
| Mojo | no | - | - | - |
| Rust (compiled) | no | - | - | - |
