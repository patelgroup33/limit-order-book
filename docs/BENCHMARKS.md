# Benchmarks

Microbenchmarks built with [Google Benchmark](https://github.com/google/benchmark).

## Running

```bash
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DLOB_BUILD_BENCHMARKS=ON
cmake --build build-bench --parallel
./build-bench/benchmark/lob_benchmarks --benchmark_min_time=0.15s
```

For stable numbers, run on an otherwise-idle machine and pin the CPU governor to
`performance`. The figures below were captured on a loaded laptop, so treat the
**absolute** latencies as indicative and read the **scaling** instead.

## Baseline (M6, `std::map` price levels)

`P` = number of distinct active price levels.

| Benchmark | P | Time | Notes |
|---|---:|---:|---|
| `BM_AddLimitOrder` | 1 | ~45 ns | rest a non-crossing order |
| `BM_AddLimitOrder` | 64 | ~52 ns | |
| `BM_AddLimitOrder` | 1024 | ~58 ns | |
| `BM_AddLimitOrder` | 16384 | ~80 ns | |
| `BM_SteadyChurn` (per add+cancel) | 64 | ~73 ns | constant-depth quote churn |
| `BM_SteadyChurn` | 1024 | ~112 ns | |
| `BM_SteadyChurn` | 16384 | ~236 ns | window < P, so most cancels empty a level |
| `BM_MatchSweep` (per fill) | 256 | ~62 ns | matching an aggressor across levels |
| `BM_MatchSweep` | 4096 | ~59 ns | |

### Reading the numbers

* **Add is logarithmic in P, not linear.** From P=1 to P=16384 (a 16,384x
  increase in levels) add time grows only ~45 ns -> ~80 ns (~1.8x). That is the
  `O(log P)` `std::map` insert, exactly as predicted.
* **Matching is linear in fills.** Per-fill cost is roughly constant (~60 ns),
  so total match time scales with the number of orders consumed -- `O(k)`.
* **Churn cost is dominated by level create/destroy at high P.** When the live
  window is smaller than P, most cancels empty their level (an `O(log P)` erase)
  and most adds create one, which is why P=16384 churn is noticeably slower.

## After (M7, direct-indexed price ladder + intrusive pool free-list)

Same machine, same command. All 49 unit tests still pass unchanged -- the swap
is behaviour-preserving.

| Benchmark | P | `std::map` (M6) | Ladder (M7) | Speedup |
|---|---:|---:|---:|---:|
| `BM_AddLimitOrder` | 1 | ~45 ns | ~49 ns | ~1.0x |
| `BM_AddLimitOrder` | 64 | ~52 ns | ~42 ns | ~1.2x |
| `BM_AddLimitOrder` | 1024 | ~58 ns | ~41 ns | ~1.4x |
| `BM_AddLimitOrder` | 16384 | ~80 ns | ~39 ns | **~2.0x** |
| `BM_SteadyChurn` | 64 | ~73 ns | ~55 ns | ~1.3x |
| `BM_SteadyChurn` | 1024 | ~112 ns | ~55 ns | ~2.0x |
| `BM_SteadyChurn` | 16384 | ~236 ns | ~55 ns | **~4.3x** |
| `BM_MatchSweep` (per fill) | 256 | ~62 ns | ~30 ns | ~2.1x |
| `BM_MatchSweep` (per fill) | 4096 | ~59 ns | ~27 ns | ~2.2x |

### What changed, and why

* **Add and churn went from logarithmic to flat.** The `std::map` cost rose with
  P (the `O(log P)` tree insert, plus node alloc/free on churn). The ladder is a
  direct array index -- `O(1)` regardless of P -- so add is ~39 ns and churn
  ~55 ns *at every P*. The high-P churn case is ~4.3x faster because the ladder
  never allocates or frees a level, and the intrusive pool free-list never
  allocates a node.
* **Matching is ~2.2x faster per fill.** Walking the book is now a linear scan of
  adjacent memory instead of chasing red-black-tree node pointers across cache
  lines. Per-fill cost fell from ~59 ns to ~27 ns.

### Remaining tradeoff

The ladder uses memory proportional to the touched price band (one `PriceLevel`
per in-band tick). `advance_best()` scans toward the next occupied level, which
is O(1) when prices cluster near the BBO but O(gap) across a sparse book; a
hierarchical bitmap for next-occupied-level lookup would bound that -- a natural
follow-up optimization.
