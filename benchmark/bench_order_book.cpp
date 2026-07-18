#include <benchmark/benchmark.h>

#include <cstdint>

#include "lob/order_book.hpp"

using namespace lob;

namespace {

// Builds a limit order. `seq` is set from the id for determinism; benchmarks
// don't rely on cross-order sequencing.
Order make_limit(std::uint64_t id, Side side, std::int64_t price, std::uint64_t qty) {
    return Order{
        .id = OrderId{id},
        .side = side,
        .type = OrderType::Limit,
        .price = Price{price},
        .quantity = Quantity{qty},
        .remaining = Quantity{qty},
        .seq = Sequence{id},
    };
}

// Spreads orders across `levels` distinct prices so the map has realistic depth.
std::int64_t price_for(std::uint64_t i, std::int64_t levels) {
    return 1000 + static_cast<std::int64_t>(i % static_cast<std::uint64_t>(levels));
}

}  // namespace

// ---------------------------------------------------------------------------
// Resting a non-crossing limit order, as a function of the number of price
// levels P. Isolates the O(log P) map insert + O(1) list append.
// ---------------------------------------------------------------------------
static void BM_AddLimitOrder(benchmark::State& state) {
    const std::int64_t levels = state.range(0);
    OrderBook book;
    std::uint64_t id = 0;
    for (auto _ : state) {
        book.add_limit_order(make_limit(id, Side::Buy, price_for(id, levels), 10));
        ++id;
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["levels"] = static_cast<double>(levels);
}
BENCHMARK(BM_AddLimitOrder)->Arg(1)->Arg(64)->Arg(1024)->Arg(16384);

// ---------------------------------------------------------------------------
// Steady-state churn: keep a fixed window of live orders, and each step cancel
// the oldest and add a new one. This is the realistic "quote churn" workload --
// book depth stays constant while orders flow through. Measures cancel (O(1))
// plus add (O(log P)) at constant depth.
// ---------------------------------------------------------------------------
static void BM_SteadyChurn(benchmark::State& state) {
    const std::int64_t levels = state.range(0);
    constexpr std::uint64_t kWindow = 8192;  // number of orders kept live

    OrderBook book;
    std::uint64_t next_id = 0;
    for (std::uint64_t i = 0; i < kWindow; ++i) {
        book.add_limit_order(make_limit(next_id, Side::Buy, price_for(next_id, levels), 10));
        ++next_id;
    }

    std::uint64_t oldest = 0;
    for (auto _ : state) {
        book.cancel(OrderId{oldest});
        ++oldest;
        book.add_limit_order(make_limit(next_id, Side::Buy, price_for(next_id, levels), 10));
        ++next_id;
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * 2);  // one cancel + one add per step
    state.counters["levels"] = static_cast<double>(levels);
}
BENCHMARK(BM_SteadyChurn)->Arg(64)->Arg(1024)->Arg(16384);

// ---------------------------------------------------------------------------
// Matching an aggressor that sweeps `levels` price levels (one fill each). The
// book is rebuilt inside a paused region so only the matching is timed.
// Demonstrates matching cost is linear in the number of fills.
// ---------------------------------------------------------------------------
static void BM_MatchSweep(benchmark::State& state) {
    const std::int64_t levels = state.range(0);
    for (auto _ : state) {
        state.PauseTiming();
        OrderBook book;
        for (std::int64_t i = 0; i < levels; ++i) {
            book.add_limit_order(make_limit(static_cast<std::uint64_t>(i), Side::Sell, 1000 + i, 1));
        }
        // A single large buy priced above every level, sized to consume them all.
        Order aggressor = make_limit(1'000'000'000ULL, Side::Buy, 1000 + levels,
                                     static_cast<std::uint64_t>(levels));
        state.ResumeTiming();

        std::size_t fills = 0;
        book.match(aggressor, [&](OrderId, Price, Quantity) { ++fills; });
        benchmark::DoNotOptimize(fills);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));  // fills processed
    state.counters["levels"] = static_cast<double>(levels);
}
BENCHMARK(BM_MatchSweep)->Arg(16)->Arg(256)->Arg(4096);

// main() is provided by benchmark::benchmark_main (linked in CMake).
