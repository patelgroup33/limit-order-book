# LOB — C++20 Limit Order Book & Matching Engine {#mainpage}

A production-inspired, single-threaded limit order book and matching engine with
price–time (FIFO) priority.

## Where to start reading

| Component | Header | Role |
|---|---|---|
| Domain types | `types.hpp` | Strong `Price` / `Quantity` / `OrderId` / `Sequence` |
| Order / Trade | `order.hpp`, `trade.hpp` | Value types on the hot path |
| Price level | `price_level.hpp` | Intrusive FIFO queue of orders at one price |
| Price ladder | `price_ladder.hpp` | Direct-indexed array of levels for one side |
| Object pool | `object_pool.hpp` | Allocation-free node recycling |
| Order book | `order_book.hpp` | Two ladders + id index + matching mechanism |
| Matching engine | `matching_engine.hpp` | Sequencing, validation, event emission |

## Design at a glance

Commands flow in (`NEW` / `CANCEL` / `MODIFY`), the engine mutates book state,
and it emits `Trade` events out. The `MatchingEngine` owns the *protocol*
(sequencing, validation, order-type semantics); the `OrderBook` owns the
*mechanism* (price-time matching over the ladders). Matching is delivered to a
caller-supplied sink, so the hot path never allocates a result container.

See `README.md` and `docs/BENCHMARKS.md` for the architecture diagram, the
`std::map` → price-ladder optimization, and benchmark numbers.
