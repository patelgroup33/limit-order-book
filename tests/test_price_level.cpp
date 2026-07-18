#include <gtest/gtest.h>

#include <deque>
#include <vector>

#include "lob/price_level.hpp"

using namespace lob;

namespace {

// Owns OrderNodes for a test. A std::deque is used on purpose: it never
// relocates existing elements on push_back, so the OrderNode* we hand to the
// PriceLevel stay valid. A std::vector would invalidate them on reallocation.
class NodeArena {
public:
    OrderNode* make(OrderId id, Quantity remaining) {
        OrderNode& node = nodes_.emplace_back();
        node.order.id = id;
        node.order.remaining = remaining;
        node.order.quantity = remaining;
        return &node;
    }

private:
    std::deque<OrderNode> nodes_;
};

// Walks the level front-to-back and returns the ids in priority order.
std::vector<std::uint64_t> ids_in_order(const PriceLevel& level) {
    std::vector<std::uint64_t> ids;
    for (const OrderNode* n = level.front(); n != nullptr; n = n->next) {
        ids.push_back(n->order.id.value());
    }
    return ids;
}

}  // namespace

TEST(PriceLevel, StartsEmpty) {
    PriceLevel level{Price{100}};
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.size(), 0u);
    EXPECT_EQ(level.total_volume().value(), 0u);
    EXPECT_EQ(level.price(), Price{100});
    EXPECT_EQ(level.front(), nullptr);
}

TEST(PriceLevel, PushBackPreservesFifoOrder) {
    NodeArena arena;
    PriceLevel level{Price{100}};

    level.push_back(arena.make(OrderId{1}, Quantity{10}));
    level.push_back(arena.make(OrderId{2}, Quantity{20}));
    level.push_back(arena.make(OrderId{3}, Quantity{5}));

    EXPECT_EQ(level.size(), 3u);
    EXPECT_EQ(level.total_volume().value(), 35u);
    EXPECT_EQ(ids_in_order(level), (std::vector<std::uint64_t>{1, 2, 3}));
    EXPECT_EQ(level.front()->order.id, OrderId{1});  // oldest = highest priority
}

TEST(PriceLevel, UnlinkFront) {
    NodeArena arena;
    PriceLevel level{Price{100}};
    OrderNode* a = arena.make(OrderId{1}, Quantity{10});
    level.push_back(a);
    level.push_back(arena.make(OrderId{2}, Quantity{20}));

    level.unlink(a);

    EXPECT_EQ(level.size(), 1u);
    EXPECT_EQ(level.total_volume().value(), 20u);
    EXPECT_EQ(ids_in_order(level), (std::vector<std::uint64_t>{2}));
    EXPECT_EQ(a->prev, nullptr);  // unlinked node's own links are cleared
    EXPECT_EQ(a->next, nullptr);
}

TEST(PriceLevel, UnlinkMiddle) {
    NodeArena arena;
    PriceLevel level{Price{100}};
    level.push_back(arena.make(OrderId{1}, Quantity{10}));
    OrderNode* mid = arena.make(OrderId{2}, Quantity{20});
    level.push_back(mid);
    level.push_back(arena.make(OrderId{3}, Quantity{5}));

    level.unlink(mid);

    EXPECT_EQ(level.size(), 2u);
    EXPECT_EQ(level.total_volume().value(), 15u);
    EXPECT_EQ(ids_in_order(level), (std::vector<std::uint64_t>{1, 3}));
}

TEST(PriceLevel, UnlinkTail) {
    NodeArena arena;
    PriceLevel level{Price{100}};
    level.push_back(arena.make(OrderId{1}, Quantity{10}));
    OrderNode* tail = arena.make(OrderId{2}, Quantity{20});
    level.push_back(tail);

    level.unlink(tail);

    EXPECT_EQ(level.size(), 1u);
    EXPECT_EQ(ids_in_order(level), (std::vector<std::uint64_t>{1}));

    // Appending after a tail-unlink must still work (tail_ was updated).
    level.push_back(arena.make(OrderId{3}, Quantity{7}));
    EXPECT_EQ(ids_in_order(level), (std::vector<std::uint64_t>{1, 3}));
}

TEST(PriceLevel, UnlinkAllReturnsToEmpty) {
    NodeArena arena;
    PriceLevel level{Price{100}};
    OrderNode* a = arena.make(OrderId{1}, Quantity{10});
    OrderNode* b = arena.make(OrderId{2}, Quantity{20});
    level.push_back(a);
    level.push_back(b);

    level.unlink(a);
    level.unlink(b);

    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.size(), 0u);
    EXPECT_EQ(level.total_volume().value(), 0u);
    EXPECT_EQ(level.front(), nullptr);
}

TEST(PriceLevel, ReduceVolumeTracksPartialFills) {
    NodeArena arena;
    PriceLevel level{Price{100}};
    OrderNode* a = arena.make(OrderId{1}, Quantity{10});
    level.push_back(a);

    // Simulate a partial fill of 4 units against the resting order.
    a->order.remaining -= Quantity{4};
    level.reduce_volume(Quantity{4});

    EXPECT_EQ(level.total_volume().value(), 6u);
    EXPECT_EQ(level.size(), 1u);  // order still resting

    // Unlink must now subtract the *current* remaining (6), landing at 0.
    level.unlink(a);
    EXPECT_EQ(level.total_volume().value(), 0u);
}
