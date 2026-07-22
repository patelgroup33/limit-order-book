#include <gtest/gtest.h>

#include <vector>

#include "backtest/analytics/metrics.hpp"
#include "backtest/data/vector_data_handler.hpp"
#include "backtest/engine.hpp"
#include "backtest/execution/simulated_execution.hpp"
#include "backtest/portfolio/portfolio.hpp"
#include "backtest/risk/risk_manager.hpp"
#include "backtest/runner.hpp"
#include "backtest/strategy/moving_average_cross.hpp"

using namespace bt;

namespace {
std::vector<Bar> bars_from(const std::vector<double>& closes) {
    std::vector<Bar> bars;
    Timestamp t = 1;
    for (const double c : closes) {
        bars.push_back(Bar{t++, c, c, c, c, 1'000'000.0});
    }
    return bars;
}
}  // namespace

TEST(BacktestRunner, RunsFullPipelineEndToEnd) {
    // MA(2,3) crosses up at t=5 (price 13) and down at t=8 (price 12).
    // With frictionless execution, the long from t=5 is closed at the t=8 flip.
    VectorDataHandler data(bars_from({13, 12, 11, 12, 13, 14, 13, 12, 11}));

    MovingAverageCross strategy(2, 3);
    Portfolio portfolio(100'000.0);
    RiskManager risk;                 // no limits
    SimulatedExecution execution;     // frictionless

    BacktestRunner runner(strategy, portfolio, risk, execution);
    BacktestEngine engine(data, runner);
    engine.run();

    // One equity point per bar.
    EXPECT_EQ(portfolio.equity_curve().size(), 9u);

    // Exactly one realized trade: the long opened at t=5 (7307 shares @ 13,
    // sized as floor(100000 * 0.95 / 13)) closed at t=8 @ 12 -> PnL = -7307.
    ASSERT_EQ(runner.trades().size(), 1u);
    EXPECT_EQ(runner.trades()[0].timestamp, 8);
    EXPECT_DOUBLE_EQ(runner.trades()[0].pnl, -7307.0);

    // The flip left a short position open.
    EXPECT_LT(portfolio.position().quantity(), 0.0);

    // Metrics can be computed from the run's outputs.
    const auto report = compute_metrics(portfolio.equity_curve(), runner.trade_pnls());
    EXPECT_EQ(report.num_trades, 1u);
    EXPECT_DOUBLE_EQ(report.total_pnl, -7307.0);
}

TEST(BacktestRunner, NoSignalsMeansNoTrades) {
    // Flat prices -> the MA never crosses -> nothing trades.
    VectorDataHandler data(bars_from({10, 10, 10, 10, 10, 10}));
    MovingAverageCross strategy(2, 3);
    Portfolio portfolio(100'000.0);
    RiskManager risk;
    SimulatedExecution execution;

    BacktestRunner runner(strategy, portfolio, risk, execution);
    BacktestEngine engine(data, runner);
    engine.run();

    EXPECT_TRUE(runner.trades().empty());
    EXPECT_TRUE(portfolio.position().is_flat());
    EXPECT_DOUBLE_EQ(portfolio.cash(), 100'000.0);
}
