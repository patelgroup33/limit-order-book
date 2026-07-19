#include <gtest/gtest.h>

#include "lob/order_book.hpp"

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
        .seq = Sequence{id.value()},
    };
}

}  // namespace

TEST(Snapshot, EmptyBookHasEmptySides) {
    OrderBook book;
    const BookSnapshot snap = book.snapshot(10);
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

TEST(Snapshot, BidsBestFirstWithAggregates) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{100}, Quantity{10}));
    book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{100}, Quantity{15}));  // same level
    book.add_limit_order(limit(OrderId{3}, Side::Buy, Price{102}, Quantity{5}));   // better
    book.add_limit_order(limit(OrderId{4}, Side::Buy, Price{101}, Quantity{7}));

    const BookSnapshot snap = book.snapshot(10);
    ASSERT_EQ(snap.bids.size(), 3u);

    EXPECT_EQ(snap.bids[0].price, Price{102});          // best (highest) first
    EXPECT_EQ(snap.bids[0].volume.value(), 5u);
    EXPECT_EQ(snap.bids[0].order_count, 1u);

    EXPECT_EQ(snap.bids[1].price, Price{101});
    EXPECT_EQ(snap.bids[1].volume.value(), 7u);

    EXPECT_EQ(snap.bids[2].price, Price{100});
    EXPECT_EQ(snap.bids[2].volume.value(), 25u);        // 10 + 15 aggregated
    EXPECT_EQ(snap.bids[2].order_count, 2u);
}

TEST(Snapshot, AsksBestFirst) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Sell, Price{110}, Quantity{10}));
    book.add_limit_order(limit(OrderId{2}, Side::Sell, Price{108}, Quantity{5}));   // better
    book.add_limit_order(limit(OrderId{3}, Side::Sell, Price{109}, Quantity{7}));

    const BookSnapshot snap = book.snapshot(10);
    ASSERT_EQ(snap.asks.size(), 3u);
    EXPECT_EQ(snap.asks[0].price, Price{108});  // best (lowest) first
    EXPECT_EQ(snap.asks[1].price, Price{109});
    EXPECT_EQ(snap.asks[2].price, Price{110});
}

TEST(Snapshot, RespectsDepthLimit) {
    OrderBook book;
    for (std::uint64_t i = 0; i < 20; ++i) {
        book.add_limit_order(
            limit(OrderId{i + 1}, Side::Buy, Price{100 - static_cast<std::int64_t>(i)}, Quantity{1}));
    }
    const BookSnapshot snap = book.snapshot(5);
    ASSERT_EQ(snap.bids.size(), 5u);
    EXPECT_EQ(snap.bids[0].price, Price{100});  // best 5 only
    EXPECT_EQ(snap.bids[4].price, Price{96});
}

TEST(Snapshot, SkipsEmptiedLevels) {
    OrderBook book;
    book.add_limit_order(limit(OrderId{1}, Side::Buy, Price{102}, Quantity{5}));
    book.add_limit_order(limit(OrderId{2}, Side::Buy, Price{101}, Quantity{5}));
    book.add_limit_order(limit(OrderId{3}, Side::Buy, Price{100}, Quantity{5}));
    book.cancel(OrderId{2});  // empties the middle level

    const BookSnapshot snap = book.snapshot(10);
    ASSERT_EQ(snap.bids.size(), 2u);
    EXPECT_EQ(snap.bids[0].price, Price{102});
    EXPECT_EQ(snap.bids[1].price, Price{100});  // 101 skipped
}
