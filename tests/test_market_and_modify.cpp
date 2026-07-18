#include <gtest/gtest.h>

#include "lob/matching_engine.hpp"

using namespace lob;

namespace {

Order limit(OrderId id, Side side, Price price, Quantity qty) {
    return Order{
        .id = id,
        .side = side,
        .type = OrderType::Limit,
        .price = price,
        .quantity = qty,
        .remaining = qty,
        .seq = Sequence{0},
    };
}

}  // namespace

// ---------------------------------------------------------------------------
// Market orders
// ---------------------------------------------------------------------------

TEST(MarketOrder, SweepsBestPricesRegardlessOfPrice) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{200}, Quantity{5}));  // far away

    auto trades = eng.submit_market_order(OrderId{3}, Side::Buy, Quantity{8});

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].price, Price{100});  // best first
    EXPECT_EQ(trades[0].quantity.value(), 5u);
    EXPECT_EQ(trades[1].price, Price{200});  // took the worse price too -- no limit
    EXPECT_EQ(trades[1].quantity.value(), 3u);
}

TEST(MarketOrder, UnfilledRemainderIsNotRested) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));

    int count = 0;
    const auto result = eng.submit_market_order(OrderId{2}, Side::Buy, Quantity{12},
                                                [&](const Trade&) { ++count; });

    EXPECT_TRUE(result.accepted);
    EXPECT_EQ(result.filled.value(), 5u);
    EXPECT_EQ(result.resting.value(), 0u);   // market never rests
    EXPECT_EQ(count, 1);
    // Nothing rested on the bid side; asks are exhausted.
    EXPECT_FALSE(eng.book().best_bid().has_value());
    EXPECT_FALSE(eng.book().best_ask().has_value());
}

TEST(MarketOrder, OnEmptyBookFillsNothing) {
    MatchingEngine eng;
    auto trades = eng.submit_market_order(OrderId{1}, Side::Buy, Quantity{5});
    EXPECT_TRUE(trades.empty());
    EXPECT_TRUE(eng.book().empty());
}

// ---------------------------------------------------------------------------
// Order modification
// ---------------------------------------------------------------------------

TEST(Modify, PureReductionKeepsTimePriority) {
    MatchingEngine eng;
    // Two resting sells at the same price; id 1 is older.
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{10}));
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{100}, Quantity{10}));

    // Reduce id 1 from 10 to 4 -- same price, pure reduction: keeps its spot.
    const auto result = eng.modify(OrderId{1}, Price{100}, Quantity{4});
    // (vector overload) -- reduction produces no trades.
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(eng.book().volume_at(Side::Sell, Price{100}).value(), 14u);  // 4 + 10

    // A buy for 4 must still hit id 1 first (priority preserved).
    auto trades = eng.submit_limit_order(limit(OrderId{3}, Side::Buy, Price{100}, Quantity{4}));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].resting, OrderId{1});
}

TEST(Modify, PriceChangeLosesPriority) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{5}));  // older
    eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{5}));  // newer

    // Move id 1 to 101 then back to 100: it re-enters at the *back* of 100.
    eng.modify(OrderId{1}, Price{101}, Quantity{5});
    eng.modify(OrderId{1}, Price{100}, Quantity{5});

    // Now a sell for 5 at 100 should hit id 2 (now the oldest at 100), not id 1.
    auto trades = eng.submit_limit_order(limit(OrderId{3}, Side::Sell, Price{100}, Quantity{5}));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].resting, OrderId{2});
}

TEST(Modify, PriceChangeCanCrossAndMatchImmediately) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));  // resting ask
    eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{99}, Quantity{5}));    // resting bid, not crossing

    // Reprice the bid up to 100: it now crosses the ask and trades.
    auto trades = eng.modify(OrderId{2}, Price{100}, Quantity{5});
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].aggressor, OrderId{2});
    EXPECT_EQ(trades[0].resting, OrderId{1});
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_TRUE(eng.book().empty());
}

TEST(Modify, SizeIncreaseLosesPriority) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));  // older
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{100}, Quantity{5}));  // newer

    // Increase id 1 from 5 to 8: size increase -> cancel/replace -> back of queue.
    eng.modify(OrderId{1}, Price{100}, Quantity{8});

    // A buy for 5 now hits id 2 first.
    auto trades = eng.submit_limit_order(limit(OrderId{3}, Side::Buy, Price{100}, Quantity{5}));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].resting, OrderId{2});
    EXPECT_EQ(eng.book().volume_at(Side::Sell, Price{100}).value(), 8u);  // id 1's new size
}

TEST(Modify, ZeroQuantityCancels) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{5}));

    const auto result = eng.modify(OrderId{1}, Price{100}, Quantity{0});
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(eng.book().contains(OrderId{1}));
}

TEST(Modify, UnknownIdIsRejected) {
    MatchingEngine eng;
    int count = 0;
    const auto result = eng.modify(OrderId{999}, Price{100}, Quantity{5},
                                   [&](const Trade&) { ++count; });
    EXPECT_FALSE(result.accepted);
    EXPECT_EQ(result.reason, RejectReason::UnknownId);
    EXPECT_EQ(count, 0);
}
