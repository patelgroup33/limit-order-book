// backtest_sim — run a backtest and export a JSON replay for the web visualizer.
//
// Usage: backtest_sim [bars] [output.json]
//   bars    number of synthetic bars to generate (default 500)
//   output  path to write (default web/backtest.json)

#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "backtest/analytics/metrics.hpp"
#include "backtest/data/vector_data_handler.hpp"
#include "backtest/engine.hpp"
#include "backtest/execution/simulated_execution.hpp"
#include "backtest/portfolio/portfolio.hpp"
#include "backtest/report/json_export.hpp"
#include "backtest/risk/risk_manager.hpp"
#include "backtest/runner.hpp"
#include "backtest/strategy/moving_average_cross.hpp"

namespace {

// A seeded random walk plus a gentle oscillation and drift — noisy enough that
// the strategy has real winners and losers (unlike a clean sine wave).
std::vector<bt::Bar> generate(std::size_t n) {
    std::mt19937 rng(42);
    std::normal_distribution<double> step(0.0, 0.6);
    std::vector<bt::Bar> bars;
    bars.reserve(n);
    double level = 100.0;
    for (std::size_t i = 0; i < n; ++i) {
        level += 0.02 + step(rng);
        double price = level + 6.0 * std::sin(static_cast<double>(i) * 0.05);
        if (price < 1.0) {
            price = 1.0;
        }
        bars.push_back(bt::Bar{static_cast<bt::Timestamp>(i + 1), price, price, price, price, 1'000'000.0});
    }
    return bars;
}

}  // namespace

int main(int argc, char** argv) {
    std::size_t n = 500;
    std::string output = "web/backtest.json";
    if (argc > 1) {
        n = static_cast<std::size_t>(std::stoul(argv[1]));
    }
    if (argc > 2) {
        output = argv[2];
    }

    const std::vector<bt::Bar> bars = generate(n);

    bt::VectorDataHandler data(bars);  // copy; `bars` is reused for the JSON
    bt::MovingAverageCross strategy(10, 30);
    bt::Portfolio portfolio(100'000.0);
    bt::RiskManager risk;
    risk.on_new_day(portfolio.initial_cash());
    bt::ExecutionConfig exec_cfg;
    exec_cfg.slippage_bps = 1.0;
    exec_cfg.commission_per_share = 0.005;
    bt::SimulatedExecution execution(exec_cfg);

    bt::BacktestRunner runner(strategy, portfolio, risk, execution);
    bt::BacktestEngine engine(data, runner);
    engine.run();

    const auto report = bt::compute_metrics(portfolio.equity_curve(), runner.trade_pnls());

    std::ofstream os(output);
    if (!os) {
        std::cerr << "error: cannot write " << output << '\n';
        return 1;
    }
    bt::write_backtest_json(os, strategy.name(), portfolio.initial_cash(), bars,
                            portfolio.equity_curve(), runner.fills(), report);

    std::cout << "wrote " << bars.size() << " bars, " << runner.fills().size() << " fills, "
              << report.num_trades << " trades to " << output << '\n';
    return 0;
}
