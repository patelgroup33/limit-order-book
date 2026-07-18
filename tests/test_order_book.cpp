#include <gtest/gtest.h>

#include "lob/order_book.hpp"

using namespace lob;

namespace {

// Builds a limit order. Quantity doubles as `remaining` inside the book.
Order limit(OrderId id, Side side, Price price, Quantity qty) {
    return Order{
        .id = id,
        .side = side,
        .type = OrderType::Limit,
        .price = price,
        .quantity = qty,
        .remaining = qty,
        .seq = Sequence{id.value()},
    };
}

}  // namespace

TEST(OrderBook, StartsEmpty) {
    OrderBook book;
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.order_count(), 0u);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(OrderBook, AddSingleBidAndAsk) {
    OrderBook book;
    EXPECT_TRUE(book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10})));
    EXPECT_TRUE(book.add_limit_order(limit(OrderId{2}, Side::Sell, Price{101}, Quantity{5})));

    EXPECT_EQ(book.order_count(), 2u);
    EXPECT_EQ(book.best_bid(), Price{100});
    EXPECT_EQ(book.best_ask(), Price{101});
    EXPECT_TRUE(book.contains(OrderId{1}));
}

TEST(OrderBook, BestPricesTrackExtremes) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10}));
    book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{102}, Quantity{10}));  // better bid
    book.add_limit_order(limit(OrderId{3}, Side::Buy, Price{101}, Quantity{10}));

    book.add_limit_order(limit(OrderId{4}, Side::Sell, Price{110}, Quantity{10}));
    book.add_limit_order(limit(OrderId{5}, Side::Sell, Price{108}, Quantity{10}));  // better ask
    book.add_limit_order(limit(OrderId{6}, Side::Sell, Price{109}, Quantity{10}));

    EXPECT_EQ(book.best_bid(), Price{102});  // highest bid
    EXPECT_EQ(book.best_ask(), Price{108});  // lowest ask
}

TEST(OrderBook, VolumeAggregatesAtPriceLevel) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10}));
    book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{15}));
    book.add_limit_order(limit(OrderId{3}, Side::Buy, Price{99}, Quantity{7}));

    EXPECT_EQ(book.volume_at(Side::Buy, Price{100}).value(), 25u);
    EXPECT_EQ(book.volume_at(Side::Buy, Price{99}).value(), 7u);
    EXPECT_EQ(book.volume_at(Side::Buy, Price{98}).value(), 0u);  // no such level
}

TEST(OrderBook, RejectsDuplicateIdAndZeroQuantity) {
    OrderBook book;
    EXPECT_TRUE(book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10})));
    EXPECT_FALSE(book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{5})));
    EXPECT_FALSE(book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{0})));
    EXPECT_EQ(book.order_count(), 1u);
}

TEST(OrderBook, CancelRemovesOrder) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10}));
    book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{15}));

    EXPECT_TRUE(book.cancel(OrderId{1}));
    EXPECT_FALSE(book.contains(OrderId{1}));
    EXPECT_EQ(book.order_count(), 1u);
    // The level still has order 2's volume.
    EXPECT_EQ(book.volume_at(Side::Buy, Price{100}).value(), 15u);
}

TEST(OrderBook, CancelLastOrderErasesLevelAndUpdatesBest) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{102}, Quantity{10}));  // best bid
    book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{101}, Quantity{10}));

    EXPECT_EQ(book.best_bid(), Price{102});
    EXPECT_TRUE(book.cancel(OrderId{1}));

    // Level 102 is now empty and erased; best bid falls to 101.
    EXPECT_EQ(book.best_bid(), Price{101});
    EXPECT_EQ(book.volume_at(Side::Buy, Price{102}).value(), 0u);
}

TEST(OrderBook, CancelUnknownIdReturnsFalse) {
    OrderBook book;
    EXPECT_FALSE(book.cancel(OrderId{999}));
}

TEST(OrderBook, CancelThenReAddRecyclesCleanly) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10}));
    EXPECT_TRUE(book.cancel(OrderId{1}));
    // Re-adding the same id must work now that the original is gone, and the
    // recycled node must not carry stale links (would corrupt the new level).
    EXPECT_TRUE(book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{20})));
    EXPECT_EQ(book.volume_at(Side::Buy, Price{100}).value(), 20u);
    EXPECT_EQ(book.order_count(), 1u);
}
