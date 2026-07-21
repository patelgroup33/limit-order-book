#include <gtest/gtest.h>

#include "backtest/portfolio/position.hpp"

using namespace bt;

TEST(Position, OpensLong) {
    Position p;
    p.apply_fill(Side::Buy, 10, 100.0);
    EXPECT_DOUBLE_EQ(p.quantity(), 10.0);
    EXPECT_DOUBLE_EQ(p.average_cost(), 100.0);
    EXPECT_DOUBLE_EQ(p.realized_pnl(), 0.0);
    EXPECT_DOUBLE_EQ(p.unrealized_pnl(110.0), 100.0);  // (110-100)*10
    EXPECT_DOUBLE_EQ(p.market_value(110.0), 1100.0);
}

TEST(Position, IncreasingLongBlendsAverageCost) {
    Position p;
    p.apply_fill(Side::Buy, 10, 100.0);
    p.apply_fill(Side::Buy, 10, 120.0);
    EXPECT_DOUBLE_EQ(p.quantity(), 20.0);
    EXPECT_DOUBLE_EQ(p.average_cost(), 110.0);  // (1000 + 1200) / 20
}

TEST(Position, ReducingLongBooksRealizedPnl) {
    Position p;
    p.apply_fill(Side::Buy, 10, 100.0);
    p.apply_fill(Side::Sell, 4, 110.0);
    EXPECT_DOUBLE_EQ(p.quantity(), 6.0);
    EXPECT_DOUBLE_EQ(p.average_cost(), 100.0);       // unchanged on a reduction
    EXPECT_DOUBLE_EQ(p.realized_pnl(), 40.0);        // (110-100)*4
}

TEST(Position, ClosingLongGoesFlat) {
    Position p;
    p.apply_fill(Side::Buy, 10, 100.0);
    p.apply_fill(Side::Sell, 10, 120.0);
    EXPECT_TRUE(p.is_flat());
    EXPECT_DOUBLE_EQ(p.average_cost(), 0.0);
    EXPECT_DOUBLE_EQ(p.realized_pnl(), 200.0);       // (120-100)*10
}

TEST(Position, FlippingLongToShort) {
    Position p;
    p.apply_fill(Side::Buy, 10, 100.0);
    p.apply_fill(Side::Sell, 15, 120.0);
    EXPECT_DOUBLE_EQ(p.quantity(), -5.0);            // flipped short
    EXPECT_DOUBLE_EQ(p.average_cost(), 120.0);       // remainder opened at fill price
    EXPECT_DOUBLE_EQ(p.realized_pnl(), 200.0);       // closed 10 at (120-100)
}

TEST(Position, ShortAndCover) {
    Position p;
    p.apply_fill(Side::Sell, 10, 100.0);
    EXPECT_DOUBLE_EQ(p.quantity(), -10.0);
    EXPECT_DOUBLE_EQ(p.average_cost(), 100.0);
    EXPECT_DOUBLE_EQ(p.unrealized_pnl(90.0), 100.0); // short profits when price drops

    p.apply_fill(Side::Buy, 4, 90.0);                // partial cover
    EXPECT_DOUBLE_EQ(p.quantity(), -6.0);
    EXPECT_DOUBLE_EQ(p.average_cost(), 100.0);
    EXPECT_DOUBLE_EQ(p.realized_pnl(), 40.0);        // (100-90)*4
}
