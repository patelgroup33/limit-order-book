#include <gtest/gtest.h>

#include "backtest/risk/risk_manager.hpp"

using namespace bt;

namespace {
OrderEvent buy(double qty) { return OrderEvent{1, Side::Buy, OrderType::Market, qty, 0.0}; }
OrderEvent sell(double qty) { return OrderEvent{1, Side::Sell, OrderType::Market, qty, 0.0}; }
}  // namespace

TEST(RiskManager, NoLimitsAcceptsEverything) {
    RiskManager rm;  // all limits disabled
    auto r = rm.check(buy(1'000'000), /*pos=*/0, /*price=*/100, /*equity=*/1e6);
    EXPECT_EQ(r.decision, RiskDecision::Accept);
    EXPECT_DOUBLE_EQ(r.quantity, 1'000'000);
}

TEST(RiskManager, MaxOrderSizeReduces) {
    RiskLimits lim;
    lim.max_order_size = 100;
    RiskManager rm(lim);
    auto r = rm.check(buy(500), 0, 100, 1e6);
    EXPECT_EQ(r.decision, RiskDecision::Reduce);
    EXPECT_DOUBLE_EQ(r.quantity, 100);
}

TEST(RiskManager, MaxPositionSizeCapsAdditions) {
    RiskLimits lim;
    lim.max_position_size = 100;
    RiskManager rm(lim);

    // Already at 80 long, buying 50 -> only 20 of room.
    auto reduce = rm.check(buy(50), /*pos=*/80, 100, 1e6);
    EXPECT_EQ(reduce.decision, RiskDecision::Reduce);
    EXPECT_DOUBLE_EQ(reduce.quantity, 20);

    // At the cap, any further add is rejected.
    auto reject = rm.check(buy(10), /*pos=*/100, 100, 1e6);
    EXPECT_EQ(reject.decision, RiskDecision::Reject);
}

TEST(RiskManager, ReducingOrdersArentBlockedByPositionCap) {
    RiskLimits lim;
    lim.max_position_size = 100;
    RiskManager rm(lim);
    // Long 100, selling 50 shrinks the position — fully allowed.
    auto r = rm.check(sell(50), /*pos=*/100, 100, 1e6);
    EXPECT_EQ(r.decision, RiskDecision::Accept);
    EXPECT_DOUBLE_EQ(r.quantity, 50);
}

TEST(RiskManager, ExposureLimitCapsByNotional) {
    RiskLimits lim;
    lim.max_exposure = 10'000;  // at price 100 -> max 100 units
    RiskManager rm(lim);
    auto r = rm.check(buy(500), /*pos=*/0, /*price=*/100, 1e6);
    EXPECT_EQ(r.decision, RiskDecision::Reduce);
    EXPECT_DOUBLE_EQ(r.quantity, 100);
}

TEST(RiskManager, TakesTighterOfPositionAndExposure) {
    RiskLimits lim;
    lim.max_position_size = 200;
    lim.max_exposure = 10'000;  // at 100 -> 100 units (tighter)
    RiskManager rm(lim);
    auto r = rm.check(buy(500), 0, 100, 1e6);
    EXPECT_EQ(r.decision, RiskDecision::Reduce);
    EXPECT_DOUBLE_EQ(r.quantity, 100);
}

TEST(RiskManager, DailyLossHaltsNewRiskButAllowsReducing) {
    RiskLimits lim;
    lim.max_daily_loss = 1'000;
    RiskManager rm(lim);
    rm.on_new_day(100'000);

    const double equity_now = 98'900;  // down 1,100 > 1,000 limit -> halted

    // Adding risk (opening long) is rejected.
    EXPECT_EQ(rm.check(buy(10), /*pos=*/0, 100, equity_now).decision, RiskDecision::Reject);
    // Adding to an existing long is rejected.
    EXPECT_EQ(rm.check(buy(10), /*pos=*/50, 100, equity_now).decision, RiskDecision::Reject);

    // Reducing a long is allowed...
    auto ok = rm.check(sell(30), /*pos=*/50, 100, equity_now);
    EXPECT_EQ(ok.decision, RiskDecision::Accept);
    EXPECT_DOUBLE_EQ(ok.quantity, 30);
    // ...but capped so it can't flip into fresh risk.
    auto capped = rm.check(sell(80), /*pos=*/50, 100, equity_now);
    EXPECT_EQ(capped.decision, RiskDecision::Reduce);
    EXPECT_DOUBLE_EQ(capped.quantity, 50);
}

TEST(RiskManager, WithinDailyLossBehavesNormally) {
    RiskLimits lim;
    lim.max_daily_loss = 1'000;
    RiskManager rm(lim);
    rm.on_new_day(100'000);
    // Only down 500 — not halted, order goes through.
    EXPECT_EQ(rm.check(buy(10), 0, 100, 99'500).decision, RiskDecision::Accept);
}
