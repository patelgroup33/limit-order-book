#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <vector>

#include "backtest/analytics/metrics.hpp"

using namespace bt;

namespace {
std::vector<EquityPoint> curve(std::initializer_list<double> equities) {
    std::vector<EquityPoint> c;
    Timestamp t = 0;
    for (const double e : equities) {
        c.push_back(EquityPoint{t++, e});
    }
    return c;
}
}  // namespace

TEST(Metrics, MaxDrawdown) {
    const auto r = compute_metrics(curve({100, 120, 90, 110}), {});
    EXPECT_DOUBLE_EQ(r.max_drawdown, 0.25);  // peak 120 -> trough 90
}

TEST(Metrics, TotalReturn) {
    const auto r = compute_metrics(curve({100, 120, 108}), {});
    EXPECT_DOUBLE_EQ(r.total_return, 0.08);
}

TEST(Metrics, SharpeSortinoAndVolatility) {
    // returns = [+0.2, -0.1]; mean 0.05, sample sd sqrt(0.045), downside dev 0.1
    const auto r = compute_metrics(curve({100, 120, 108}), {});
    EXPECT_NEAR(r.annualized_volatility, 3.3675, 1e-3);
    EXPECT_NEAR(r.sharpe_ratio, 3.742, 1e-3);
    EXPECT_NEAR(r.sortino_ratio, 7.937, 1e-3);   // downside dev < total sd -> Sortino > Sharpe
    EXPECT_NEAR(r.annualized_return, 12.6, 1e-9); // 0.05 * 252
}

TEST(Metrics, CagrOverOneYear) {
    MetricsConfig cfg;
    cfg.periods_per_year = 2.0;  // 2 returns / 2 per year = 1 year
    const auto r = compute_metrics(curve({100, 120, 108}), {}, cfg);
    EXPECT_NEAR(r.cagr, 0.08, 1e-9);  // over exactly one year, CAGR == total return
}

TEST(Metrics, TradeStatistics) {
    const std::vector<double> pnls = {100, -50, 200, -30, 0};
    const auto r = compute_metrics({}, pnls);

    EXPECT_EQ(r.num_trades, 5u);
    EXPECT_EQ(r.num_wins, 2u);
    EXPECT_EQ(r.num_losses, 2u);
    EXPECT_DOUBLE_EQ(r.win_rate, 0.4);
    EXPECT_DOUBLE_EQ(r.profit_factor, 300.0 / 80.0);  // 3.75
    EXPECT_DOUBLE_EQ(r.avg_win, 150.0);
    EXPECT_DOUBLE_EQ(r.avg_loss, -40.0);
    EXPECT_DOUBLE_EQ(r.largest_win, 200.0);
    EXPECT_DOUBLE_EQ(r.largest_loss, -50.0);
    EXPECT_DOUBLE_EQ(r.total_pnl, 220.0);
}

TEST(Metrics, ProfitFactorInfiniteWithNoLosses) {
    const auto r = compute_metrics({}, {10.0, 20.0});
    EXPECT_TRUE(std::isinf(r.profit_factor));
}

TEST(Metrics, EmptyInputsAreZeroNotNan) {
    const auto r = compute_metrics({}, {});
    EXPECT_EQ(r.num_trades, 0u);
    EXPECT_DOUBLE_EQ(r.sharpe_ratio, 0.0);
    EXPECT_DOUBLE_EQ(r.max_drawdown, 0.0);
    EXPECT_DOUBLE_EQ(r.total_return, 0.0);
}
