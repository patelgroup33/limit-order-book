#include <gtest/gtest.h>

#include <type_traits>

#include "lob/order.hpp"
#include "lob/trade.hpp"

using namespace lob;

// Order and Trade are pure data on the hot path -- they must stay trivially
// copyable so the engine can shuffle them around for free.
static_assert(std::is_trivially_copyable_v<Order>);
static_assert(std::is_trivially_copyable_v<Trade>);

TEST(Order, DefaultsAndFilledPredicate) {
    Order o{
        .id = OrderId{1},
        .side = Side::Buy,
        .type = OrderType::Limit,
        .price = Price{100},
        .quantity = Quantity{50},
        .remaining = Quantity{50},
        .seq = Sequence{1},
    };
    EXPECT_FALSE(o.is_filled());

    o.remaining -= Quantity{50};
    EXPECT_TRUE(o.is_filled());
}

TEST(Trade, HoldsExecutionRecord) {
    const Trade t{
        .aggressor = OrderId{2},
        .resting = OrderId{1},
        .price = Price{100},
        .quantity = Quantity{25},
        .seq = Sequence{3},
    };
    EXPECT_EQ(t.aggressor, OrderId{2});
    EXPECT_EQ(t.resting, OrderId{1});
    EXPECT_EQ(t.price, Price{100});
    EXPECT_EQ(t.quantity.value(), 25u);
}
