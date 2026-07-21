#pragma once

#include <variant>

#include "backtest/core/bar.hpp"
#include "backtest/core/enums.hpp"

namespace bt {

// A new bar is available. Drives the whole loop.
struct MarketEvent {
    Bar bar;
};

// A strategy's directional intent for the instrument, before sizing.
struct SignalEvent {
    Timestamp  timestamp{0};
    SignalType type{SignalType::Exit};
    double     strength{1.0};  // 0..1 confidence; the portfolio uses it to size
};

// An order handed to (simulated) execution.
struct OrderEvent {
    Timestamp timestamp{0};
    Side      side{Side::Buy};
    OrderType type{OrderType::Market};
    double    quantity{0.0};
    double    limit_price{0.0};  // only meaningful for limit orders
};

// The result of executing an order.
struct FillEvent {
    Timestamp timestamp{0};
    Side      side{Side::Buy};
    double    quantity{0.0};
    double    price{0.0};
    double    commission{0.0};
};

// Events are value types held in a variant — no per-event heap allocation, which
// matters at millions of events. Dispatch is a std::visit, not virtual calls.
using Event = std::variant<MarketEvent, SignalEvent, OrderEvent, FillEvent>;

}  // namespace bt
