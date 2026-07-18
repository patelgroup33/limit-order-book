#include <gtest/gtest.h>

#include <vector>

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
        .seq = Sequence{0},  // the engine stamps the real sequence
    };
}

}  // namespace

TEST(MatchingEngine, NonCrossingOrderRestsWithoutTrade) {
    MatchingEngine eng;
    auto trades = eng.submit_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10}));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(eng.book().best_bid(), Price{100});
    EXPECT_EQ(eng.book().volume_at(Side::Buy, Price{100}).value(), 10u);
}

TEST(MatchingEngine, FullFillAgainstSingleResting) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));  // resting ask
    auto trades = eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{5}));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].aggressor, OrderId{2});
    EXPECT_EQ(trades[0].resting, OrderId{1});
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_EQ(trades[0].quantity.value(), 5u);

    // Both orders gone; book is empty.
    EXPECT_TRUE(eng.book().empty());
    EXPECT_FALSE(eng.book().best_ask().has_value());
}

TEST(MatchingEngine, ExecutionPriceIsRestingPrice) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));   // ask 100
    auto trades = eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{105}, Quantity{5}));  // willing to pay 105

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, Price{100});  // price improvement: trades at 100, not 105
}

TEST(MatchingEngine, AggressorPartialFillRestsLeftover) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));
    auto trades = eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{8}));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity.value(), 5u);

    // 3 units couldn't fill and now rest as a bid at 100.
    EXPECT_FALSE(eng.book().best_ask().has_value());
    EXPECT_EQ(eng.book().best_bid(), Price{100});
    EXPECT_EQ(eng.book().volume_at(Side::Buy, Price{100}).value(), 3u);
}

TEST(MatchingEngine, RestingPartialFillStays) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{10}));
    auto trades = eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{4}));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity.value(), 4u);

    // Resting ask keeps 6 and stays; aggressor fully filled, nothing rests.
    EXPECT_FALSE(eng.book().best_bid().has_value());
    EXPECT_EQ(eng.book().volume_at(Side::Sell, Price{100}).value(), 6u);
}

TEST(MatchingEngine, SweepsMultipleLevelsBestPriceFirst) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));   // best ask
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{101}, Quantity{5}));

    auto trades = eng.submit_limit_order(limit(OrderId{3}, Side::Buy, Price{101}, Quantity{8}));

    ASSERT_EQ(trades.size(), 2u);
    // First fill at the better price (100), then the next level (101).
    EXPECT_EQ(trades[0].resting, OrderId{1});
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_EQ(trades[0].quantity.value(), 5u);
    EXPECT_EQ(trades[1].resting, OrderId{2});
    EXPECT_EQ(trades[1].price, Price{101});
    EXPECT_EQ(trades[1].quantity.value(), 3u);

    // Level 100 cleared; 101 has 2 left; aggressor fully filled.
    EXPECT_EQ(eng.book().best_ask(), Price{101});
    EXPECT_EQ(eng.book().volume_at(Side::Sell, Price{101}).value(), 2u);
}

TEST(MatchingEngine, FifoWithinAPriceLevel) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));  // older
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{100}, Quantity{5}));  // newer

    auto trades = eng.submit_limit_order(limit(OrderId{3}, Side::Buy, Price{100}, Quantity{5}));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].resting, OrderId{1});  // oldest at the level fills first
    EXPECT_EQ(eng.book().volume_at(Side::Sell, Price{100}).value(), 5u);  // id 2 remains
}

TEST(MatchingEngine, SellAggressorMatchesBids) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{5}));   // resting bid
    auto trades = eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{100}, Quantity{5}));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].aggressor, OrderId{2});
    EXPECT_EQ(trades[0].resting, OrderId{1});
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_TRUE(eng.book().empty());
}

TEST(MatchingEngine, TradeSequenceNumbersAreMonotonic) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{101}, Quantity{5}));
    auto trades = eng.submit_limit_order(limit(OrderId{3}, Side::Buy, Price{101}, Quantity{10}));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_LT(trades[0].seq, trades[1].seq);  // events are strictly ordered
}

TEST(MatchingEngine, RejectsZeroQuantityAndDuplicateId) {
    MatchingEngine eng;
    const auto zero = eng.submit_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{0}),
                                             [](const Trade&) {});
    EXPECT_FALSE(zero.accepted);
    EXPECT_EQ(zero.reason, RejectReason::ZeroQuantity);

    EXPECT_TRUE(eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{5}),
                                       [](const Trade&) {})
                    .accepted);

    const auto dup = eng.submit_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{5}),
                                            [](const Trade&) {});
    EXPECT_FALSE(dup.accepted);
    EXPECT_EQ(dup.reason, RejectReason::DuplicateId);
}

TEST(MatchingEngine, SinkOverloadStreamsEachTrade) {
    MatchingEngine eng;
    eng.submit_limit_order(limit(OrderId{1}, Side::Sell, Price{100}, Quantity{5}));
    eng.submit_limit_order(limit(OrderId{2}, Side::Sell, Price{101}, Quantity{5}));

    int count = 0;
    Quantity total{0};
    const auto result = eng.submit_limit_order(
        limit(OrderId{3}, Side::Buy, Price{101}, Quantity{10}),
        [&](const Trade& t) {
            ++count;
            total += t.quantity;
        });

    EXPECT_TRUE(result.accepted);
    EXPECT_EQ(result.filled.value(), 10u);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(total.value(), 10u);
}
