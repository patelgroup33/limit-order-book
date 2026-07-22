// End-to-end example: run a moving-average-cross strategy through the full
// backtest pipeline and print a performance summary.
//
// Usage: backtest_example [data.csv]
//   With no argument it generates a synthetic trending/oscillating series so it
//   runs anywhere; pass a CSV (timestamp,open,high,low,close,volume) to use real
//   data.

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "backtest/analytics/metrics.hpp"
#include "backtest/data/csv_data_handler.hpp"
#include "backtest/data/vector_data_handler.hpp"
#include "backtest/engine.hpp"
#include "backtest/execution/simulated_execution.hpp"
#include "backtest/portfolio/portfolio.hpp"
#include "backtest/report/reporter.hpp"
#include "backtest/risk/risk_manager.hpp"
#include "backtest/runner.hpp"
#include "backtest/strategy/moving_average_cross.hpp"

namespace {

std::vector<bt::Bar> synthetic_series(std::size_t n) {
    std::vector<bt::Bar> bars;
    bars.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i);
        const double price = 100.0 + 15.0 * std::sin(t * 0.08) + 0.03 * t;  // oscillate + drift
        bars.push_back(bt::Bar{static_cast<bt::Timestamp>(i + 1), price, price, price, price, 1'000'000.0});
    }
    return bars;
}

}  // namespace

int main(int argc, char** argv) {
    // 1. Data.
    std::unique_ptr<bt::DataHandler> data;
    if (argc > 1) {
        data = std::make_unique<bt::CsvDataHandler>(argv[1]);
    } else {
        data = std::make_unique<bt::VectorDataHandler>(synthetic_series(750));
    }

    // 2. Components.
    bt::MovingAverageCross strategy(10, 30);
    bt::Portfolio portfolio(100'000.0);

    bt::RiskLimits limits;
    limits.max_exposure = 150'000.0;  // don't lever past 1.5x
    bt::RiskManager risk(limits);
    risk.on_new_day(portfolio.initial_cash());

    bt::ExecutionConfig exec_cfg;
    exec_cfg.slippage_bps = 1.0;          // 1 bp
    exec_cfg.commission_per_share = 0.005;
    bt::SimulatedExecution execution(exec_cfg);

    // 3. Wire and run.
    bt::BacktestRunner runner(strategy, portfolio, risk, execution);
    bt::BacktestEngine engine(*data, runner);
    engine.run();

    // 4. Metrics + report.
    const auto report = bt::compute_metrics(portfolio.equity_curve(), runner.trade_pnls());
    bt::write_summary(std::cout, report);

    {
        std::ofstream eq("equity_curve.csv");
        bt::write_equity_curve_csv(eq, portfolio.equity_curve());
        std::ofstream tr("trades.csv");
        bt::write_trades_csv(tr, runner.trades());
    }
    std::cout << "\nWrote equity_curve.csv and trades.csv\n";
    return 0;
}
