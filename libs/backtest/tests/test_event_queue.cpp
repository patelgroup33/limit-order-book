#include <gtest/gtest.h>

#include <vector>

#include "backtest/event/event_queue.hpp"

using namespace bt;

namespace {
// Use an OrderEvent's quantity as a marker so we can identify events after pop.
Event marker(double id) {
    return OrderEvent{0, Side::Buy, OrderType::Market, id, 0.0};
}
double marker_id(const Event& e) { return std::get<OrderEvent>(e).quantity; }
}  // namespace

TEST(EventQueue, StartsEmpty) {
    EventQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(EventQueue, PopsInTimeOrderWithFifoTiebreak) {
    EventQueue q;
    q.push(marker(10), 3);
    q.push(marker(20), 1);
    q.push(marker(30), 1);  // same time as 20, inserted after -> FIFO after it
    q.push(marker(40), 2);

    EXPECT_EQ(q.size(), 4u);
    EXPECT_EQ(q.next_time(), 1);

    std::vector<double> order;
    while (!q.empty()) {
        order.push_back(marker_id(q.pop()));
    }
    // time 1 (20 then 30), then time 2 (40), then time 3 (10)
    EXPECT_EQ(order, (std::vector<double>{20, 30, 40, 10}));
    EXPECT_TRUE(q.empty());
}

TEST(EventQueue, FutureEventStaysUntilItsTime) {
    EventQueue q;
    q.push(marker(1), 5);   // scheduled in the "future"
    q.push(marker(2), 2);   // sooner
    EXPECT_EQ(q.next_time(), 2);
    EXPECT_EQ(marker_id(q.pop()), 2);
    EXPECT_EQ(q.next_time(), 5);
    EXPECT_EQ(marker_id(q.pop()), 1);
}
