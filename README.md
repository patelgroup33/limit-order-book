# Limit Order Book

[![CI](https://github.com/patelgroup33/limit-order-book/actions/workflows/ci.yml/badge.svg)](https://github.com/patelgroup33/limit-order-book/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A limit order book and matching engine in C++. It handles limit and market
orders, partial fills, cancels, and modifies, using price–time (FIFO) priority.

I built this to understand the data structures behind an exchange matching core
and to have something real to profile and optimize. The price levels started out
in a `std::map`; after measuring, I moved them to a flat tick-indexed array,
which is where most of the speedup came from.

## Layout

```
include/lob/   headers
src/           library sources
tests/         GoogleTest suite
benchmark/     Google Benchmark microbenchmarks
sim/           lob_sim: order-flow simulator + JSON export
web/           browser visualizer + generated replay data
scripts/       build / test / run helpers (.sh + .bat)
docs/          Doxygen config + benchmark notes
```

## Building

```bash
scripts/build.sh        # configure + build (library, tests, simulator)
scripts/test.sh         # build + run the tests
scripts/run_sim.sh      # build the simulator, generate data, open the visualizer
```

Or with CMake directly:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Needs a C++20 compiler (GCC 11+, Clang 14+, Apple Clang 14+) and CMake 3.20+.
GoogleTest and Google Benchmark are fetched during configure. Benchmarks and
docs are behind `-DLOB_BUILD_BENCHMARKS=ON` and `-DLOB_BUILD_DOCS=ON`.

## How it works

- Orders resting at one price sit in a FIFO doubly-linked list (`PriceLevel`).
  The list is intrusive and the nodes come from an object pool, so adding and
  cancelling don't hit the allocator.
- Each side of the book is an array of those levels indexed by price
  (`PriceLadder`). The best price is a cached index, so best-bid/ask is O(1).
- An `unordered_map<OrderId, node*>` gives O(1) cancel and modify — look up the
  node, unlink it, done.
- `MatchingEngine` sits on top: it assigns sequence numbers, validates, runs the
  match, rests any remainder, and emits trades to a callback.

Prices are integers (ticks), never floats — you can't reliably compare floating
point for equality, and matching needs that. "Time" is a monotonic sequence
counter rather than a clock, which keeps runs deterministic and replayable.

## Performance

Moving the price levels from `std::map` to the tick-indexed ladder:

| Operation | `std::map` | Ladder |
|---|---:|---:|
| Add limit order (16k levels) | ~80 ns | ~39 ns |
| Add/cancel churn (16k levels) | ~236 ns | ~55 ns |
| Match, per fill | ~59 ns | ~27 ns |

The map is O(log P) and its nodes are scattered in memory; the array is O(1) and
contiguous, so walking the book stops chasing pointers. Numbers are from a
laptop under load — the scaling matters more than the absolute nanoseconds.
Details in [docs/BENCHMARKS.md](docs/BENCHMARKS.md).

## Visualizer

`lob_sim` runs the engine against synthetic order flow and writes a replay to
`web/simulation.json`. `web/index.html` plays it back — a depth ladder, a
market-depth chart, a trade tape, and play/pause/scrub controls. `run_sim.sh`
builds it, regenerates the data, and serves it at `http://localhost:8137`. A
GitHub Actions workflow publishes `web/` to Pages; see [web/README.md](web/README.md).

## Notes

Git hooks live in `.githooks/` — activate them once per clone with
`git config core.hooksPath .githooks`.

Things I'd add next: self-trade prevention, iceberg and stop orders, a bitmap to
bound the best-price scan when the book is sparse, and multiple instruments.

## License

MIT — see [LICENSE](LICENSE).
