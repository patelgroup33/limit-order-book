#include <benchmark/benchmark.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "backtest/data/csv_data_handler.hpp"
#include "backtest/data/vector_data_handler.hpp"
#include "backtest/engine.hpp"
#include "backtest/execution/simulated_execution.hpp"
#include "backtest/portfolio/portfolio.hpp"
#include "backtest/risk/risk_manager.hpp"
#include "backtest/runner.hpp"
#include "backtest/strategy/moving_average_cross.hpp"

namespace {

std::vector<bt::Bar> generate(std::size_t n) {
    std::vector<bt::Bar> bars;
    bars.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i);
        const double price = 100.0 + 10.0 * std::sin(t * 0.01) + 0.0001 * t;
        bars.push_back(bt::Bar{static_cast<bt::Timestamp>(i + 1), price, price, price, price, 1'000'000.0});
    }
    return bars;
}

}  // namespace

// Full backtest throughput: strategy + risk + execution + portfolio over N bars.
static void BM_BacktestRun(benchmark::State& state) {
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    bt::VectorDataHandler data(generate(n));  // built once; reset each iteration
    for (auto _ : state) {
        data.reset();
        bt::MovingAverageCross strategy(10, 30);
        bt::Portfolio portfolio(100'000.0);
        bt::RiskManager risk;
        bt::SimulatedExecution execution;
        bt::BacktestRunner runner(strategy, portfolio, risk, execution);
        bt::BacktestEngine engine(data, runner);
        engine.run();
        benchmark::DoNotOptimize(portfolio.equity_curve().size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}
BENCHMARK(BM_BacktestRun)->Arg(100'000)->Arg(1'000'000)->Arg(10'000'000)->Unit(benchmark::kMillisecond);

// CSV parsing throughput (the data-loading hot path).
static void BM_CsvParse(benchmark::State& state) {
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    std::string csv = "timestamp,open,high,low,close,volume\n";
    for (std::size_t i = 0; i < n; ++i) {
        csv += std::to_string(i + 1) + ",100,101,99,100.5,1000\n";
    }
    for (auto _ : state) {
        bt::CsvOptions opts;
        auto handler = bt::CsvDataHandler::from_text(csv, opts);
        benchmark::DoNotOptimize(handler.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}
BENCHMARK(BM_CsvParse)->Arg(100'000)->Arg(1'000'000)->Unit(benchmark::kMillisecond);
