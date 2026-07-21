#include <gtest/gtest.h>

#include "backtest/execution/simulated_execution.hpp"

using namespace bt;

namespace {
Bar market(double close, double volume = 1'000.0) {
    return Bar{100, close, close, close, close, volume};
}
OrderEvent order(Side side, OrderType type, double qty, double limit = 0.0, Timestamp t = 100) {
    return OrderEvent{t, side, type, qty, limit};
}
}  // namespace

TEST(SimulatedExecution, MarketOrderFillsAtCloseWhenFrictionless) {
    SimulatedExecution exec;  // no costs
    auto fill = exec.execute(order(Side::Buy, OrderType::Market, 10), market(100.0));
    ASSERT_TRUE(fill.has_value());
    EXPECT_DOUBLE_EQ(fill->price, 100.0);
    EXPECT_DOUBLE_EQ(fill->quantity, 10.0);
    EXPECT_DOUBLE_EQ(fill->commission, 0.0);
    EXPECT_EQ(fill->side, Side::Buy);
}

TEST(SimulatedExecution, SlippageMovesPriceAgainstYou) {
    ExecutionConfig cfg;
    cfg.slippage_bps = 10.0;  // 0.1%
    SimulatedExecution exec(cfg);

    auto buy = exec.execute(order(Side::Buy, OrderType::Market, 10), market(100.0));
    auto sell = exec.execute(order(Side::Sell, OrderType::Market, 10), market(100.0));
    ASSERT_TRUE(buy && sell);
    EXPECT_DOUBLE_EQ(buy->price, 100.1);   // buy pays up
    EXPECT_DOUBLE_EQ(sell->price, 99.9);   // sell receives less
}

TEST(SimulatedExecution, CommissionPerShareAndRateWithFloor) {
    ExecutionConfig cfg;
    cfg.commission_per_share = 0.005;  // 0.5c per share
    cfg.commission_rate = 0.001;       // 10 bps of notional
    cfg.min_commission = 1.0;
    SimulatedExecution exec(cfg);

    // 100 @ 100: per-share 0.5 + rate 0.001*10000 = 10 -> 10.5, above the floor.
    auto fill = exec.execute(order(Side::Buy, OrderType::Market, 100), market(100.0));
    ASSERT_TRUE(fill.has_value());
    EXPECT_DOUBLE_EQ(fill->commission, 0.5 + 10.0);

    // Tiny order hits the minimum-commission floor.
    auto tiny = exec.execute(order(Side::Buy, OrderType::Market, 1), market(1.0));
    ASSERT_TRUE(tiny.has_value());
    EXPECT_DOUBLE_EQ(tiny->commission, 1.0);
}

TEST(SimulatedExecution, LimitBuyOnlyFillsWhenMarketable) {
    SimulatedExecution exec;
    // Market at 100, limit buy at 99: not marketable (won't pay 100).
    EXPECT_FALSE(exec.execute(order(Side::Buy, OrderType::Limit, 10, 99.0), market(100.0)).has_value());
    // Limit buy at 101: marketable, fills at the market price 100.
    auto fill = exec.execute(order(Side::Buy, OrderType::Limit, 10, 101.0), market(100.0));
    ASSERT_TRUE(fill.has_value());
    EXPECT_DOUBLE_EQ(fill->price, 100.0);
}

TEST(SimulatedExecution, LimitSellOnlyFillsWhenMarketable) {
    SimulatedExecution exec;
    EXPECT_FALSE(exec.execute(order(Side::Sell, OrderType::Limit, 10, 101.0), market(100.0)).has_value());
    auto fill = exec.execute(order(Side::Sell, OrderType::Limit, 10, 99.0), market(100.0));
    ASSERT_TRUE(fill.has_value());
    EXPECT_DOUBLE_EQ(fill->price, 100.0);
}

TEST(SimulatedExecution, PartialFillCappedByVolume) {
    ExecutionConfig cfg;
    cfg.max_volume_fraction = 0.1;  // can take 10% of bar volume
    SimulatedExecution exec(cfg);
    // Bar volume 1000 -> cap 100. Order for 500 fills only 100.
    auto fill = exec.execute(order(Side::Buy, OrderType::Market, 500), market(100.0, 1'000.0));
    ASSERT_TRUE(fill.has_value());
    EXPECT_DOUBLE_EQ(fill->quantity, 100.0);
}

TEST(SimulatedExecution, LatencyDelaysFillTimestamp) {
    ExecutionConfig cfg;
    cfg.latency = 3;
    SimulatedExecution exec(cfg);
    auto fill = exec.execute(order(Side::Buy, OrderType::Market, 10, 0.0, /*t=*/100), market(100.0));
    ASSERT_TRUE(fill.has_value());
    EXPECT_EQ(fill->timestamp, 103);
}
