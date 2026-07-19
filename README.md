# LOB — A C++20 Limit Order Book & Matching Engine

[![CI](https://github.com/patelgroup33/limit-order-book/actions/workflows/ci.yml/badge.svg)](https://github.com/patelgroup33/limit-order-book/actions/workflows/ci.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A production-inspired, single-threaded **limit order book and matching engine**
in modern C++20, with **price–time (FIFO) priority** and the data structures a
real exchange matching core uses. Built incrementally, tested at every step, and
optimized against its own benchmarks.

---

## Highlights

- **Price–time priority** matching; limit **and** market orders
- **Partial fills**, order **cancellation**, and **modification** (with correct
  time-priority semantics: pure reductions keep priority, reprices/size-increases
  go to the back of the queue)
- **O(1)** order lookup for cancel/modify via an `OrderId → node` index
- **Direct-indexed price ladder** for O(1), cache-friendly level access
- **Intrusive linked lists** + an **object pool** → *zero heap allocation* on the
  add/cancel hot path
- Strong types (`Price`/`Quantity`/`OrderId`) that make argument-swap bugs
  *compile errors*, at zero runtime cost
- Streaming (allocation-free) trade-event API, with a convenience `vector` API
- **CMake** build, **GoogleTest** suite (49 tests), **Google Benchmark**
  microbenchmarks, **Doxygen** docs, **GitHub Actions** CI

## Architecture

```
   Commands ─►  MatchingEngine ─────────────────────────────► Trade events
   NEW/CXL/MOD    (protocol: sequencing, validation,           (streamed to
                   order-type semantics, event emission)        a sink)
                        │
                        ▼
                   OrderBook  (mechanism: price-time matching)
                   ┌──────────────────────────────────────────┐
                   │  bids:  PriceLadder  (best = highest)     │
                   │  asks:  PriceLadder  (best = lowest)      │
                   │  index: unordered_map<OrderId, node*>     │  ← O(1) cancel
                   │  pool:  ObjectPool<OrderNode>             │  ← no malloc
                   └──────────────────────────────────────────┘
                        │
                        ▼
                   PriceLevel  (intrusive FIFO list of orders at one price)
```

The engine talks to the book through a narrow interface, which is what let the
price-level container be swapped from `std::map` to a flat ladder in Milestone 7
**without touching matching logic or a single test** (see below).

## Performance

Swapping the price-level container from `std::map` (red-black tree) to a
direct-indexed **price ladder** changed the complexity *class* of the hot paths
and cut match latency roughly in half — with all 49 tests passing unchanged.

| Operation | `std::map` | Price ladder | Result |
|---|---:|---:|---|
| Add limit order (16,384 levels) | ~80 ns | ~39 ns | rising `O(log P)` → **flat `O(1)`** |
| Add/cancel churn (16,384 levels) | ~236 ns | ~55 ns | **~4.3× faster** |
| Match, per fill | ~59 ns | ~27 ns | **~2.2× faster** (cache locality) |

Full methodology and before/after tables: [docs/BENCHMARKS.md](docs/BENCHMARKS.md).
*(Numbers are from a loaded laptop — read the scaling, not the absolute ns.)*

## Live simulation & visualizer

The `lob_sim` executable drives the **real matching engine** with synthetic
order flow and records a deterministic replay (`web/simulation.json`). A
dependency-free web page (`web/index.html`) replays it: an L2 order-book ladder,
a cumulative market-depth chart, a time-&-sales tape, a price sparkline, and
play/pause/scrub/speed controls.

```bash
cmake --build build --target lob_sim
./build/sim/lob_sim 1200 web/simulation.json    # (re)generate the replay
cd web && python3 -m http.server 8137           # open http://localhost:8137
```

A GitHub Actions workflow publishes `web/` to GitHub Pages (enable **Settings →
Pages → Source: GitHub Actions**), giving a shareable demo at
`https://patelgroup33.github.io/limit-order-book/`. See [web/README.md](web/README.md).

## Building

```bash
cmake -S . -B build                              # configure (Release by default)
cmake --build build --parallel                   # build library + tests
ctest --test-dir build --output-on-failure       # run the 49-test suite
```

Optional targets:

```bash
# Benchmarks
cmake -S . -B build-bench -DLOB_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --parallel
./build-bench/benchmark/lob_benchmarks

# API docs (requires doxygen) -> docs/html/index.html
cmake -S . -B build-docs -DLOB_BUILD_DOCS=ON
cmake --build build-docs --target docs
```

**Requirements:** a C++20 compiler (GCC 11+, Clang 14+, Apple Clang 14+) and
CMake 3.20+. GoogleTest and Google Benchmark are fetched automatically.

## Project layout

```
include/lob/   public headers (one concept per header)
src/           library sources
tests/         GoogleTest suite
benchmark/     Google Benchmark microbenchmarks
sim/           lob_sim: order-flow simulator + JSON export
web/           browser visualizer (index.html) + generated replay data
docs/          Doxygen config, mainpage, benchmark report
.github/       CI + GitHub Pages workflows
```

## Design decisions worth a look

- **Integers, never floats, for price.** IEEE-754 can't compare equal reliably
  and isn't associative — fatal for FIFO priority and deterministic replay.
- **Sequence numbers, not timestamps.** A single monotonic counter makes the
  engine deterministic and replayable (feed the same commands → identical trades).
- **Engine owns protocol, book owns mechanism.** The split is what makes the
  storage swappable and the matching testable in isolation.
- **Measure, then optimize.** The `std::map` baseline was benchmarked *first*, so
  the ladder's win is proven, not assumed.

## Roadmap

- [x] **M0** — Project scaffold (build system, test harness, CI)
- [x] **M1** — Domain types (strong `Price`/`Quantity`/`OrderId`, `Order`, `Trade`)
- [x] **M2** — `PriceLevel` with intrusive FIFO list
- [x] **M3** — `OrderBook`: add & cancel resting limit orders
- [x] **M4** — Matching engine: limit orders, partial fills, trades
- [x] **M5** — Market orders + order modification + result reporting
- [x] **M6** — Benchmarks
- [x] **M7** — Performance optimization (`std::map` → price ladder)
- [x] **M8** — Polish (Doxygen, README, CI badge)

### Possible extensions

Self-trade prevention (STP), iceberg/stop orders, a hierarchical bitmap for
`advance_best()`, a FIX-style gateway, and multi-instrument sharding.

## License

[MIT](LICENSE).
