#include <gtest/gtest.h>

#include "backtest/portfolio/portfolio.hpp"

using namespace bt;

namespace {
FillEvent fill(Side side, double qty, double price, double commission = 0.0) {
    return FillEvent{1, side, qty, price, commission};
}
}  // namespace

TEST(Portfolio, StartsWithCashAndFlatEquity) {
    Portfolio pf(100'000.0);
    EXPECT_DOUBLE_EQ(pf.cash(), 100'000.0);
    EXPECT_DOUBLE_EQ(pf.buying_power(), 100'000.0);
    EXPECT_DOUBLE_EQ(pf.equity(123.45), 100'000.0);  // no position: equity == cash
    EXPECT_TRUE(pf.position().is_flat());
}

TEST(Portfolio, BuySpendsCashAndTakesPosition) {
    Portfolio pf(100'000.0);
    pf.on_fill(fill(Side::Buy, 100, 50.0, /*commission=*/1.0));
    EXPECT_DOUBLE_EQ(pf.cash(), 100'000.0 - 5000.0 - 1.0);
    EXPECT_DOUBLE_EQ(pf.position().quantity(), 100.0);
    // equity = cash + position value = (94999) + 100*50 = 99999 (down by commission)
    EXPECT_DOUBLE_EQ(pf.equity(50.0), 99'999.0);
}

TEST(Portfolio, EquityTracksUnrealizedPnl) {
    Portfolio pf(100'000.0);
    pf.on_fill(fill(Side::Buy, 100, 50.0));
    EXPECT_DOUBLE_EQ(pf.unrealized_pnl(60.0), 1000.0);   // (60-50)*100
    EXPECT_DOUBLE_EQ(pf.equity(60.0), 101'000.0);        // 95000 cash + 6000 position
}

TEST(Portfolio, SellRealizesPnlIntoCash) {
    Portfolio pf(100'000.0);
    pf.on_fill(fill(Side::Buy, 100, 50.0));
    pf.on_fill(fill(Side::Sell, 100, 60.0));
    EXPECT_TRUE(pf.position().is_flat());
    EXPECT_DOUBLE_EQ(pf.realized_pnl(), 1000.0);
    EXPECT_DOUBLE_EQ(pf.cash(), 101'000.0);              // back to cash, up 1000
    EXPECT_DOUBLE_EQ(pf.equity(999.0), 101'000.0);       // flat: equity == cash at any price
}

TEST(Portfolio, MarkRecordsEquityCurve) {
    Portfolio pf(100'000.0);
    pf.on_fill(fill(Side::Buy, 100, 50.0));
    pf.mark(10, 55.0);
    pf.mark(20, 60.0);
    ASSERT_EQ(pf.equity_curve().size(), 2u);
    EXPECT_EQ(pf.equity_curve()[0].timestamp, 10);
    EXPECT_DOUBLE_EQ(pf.equity_curve()[0].equity, 100'500.0);  // 95000 + 100*55
    EXPECT_DOUBLE_EQ(pf.equity_curve()[1].equity, 101'000.0);  // 95000 + 100*60
}
