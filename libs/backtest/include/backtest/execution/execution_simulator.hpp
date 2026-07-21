#pragma once

#include <optional>

#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"

namespace bt {

// Turns an order into a fill against the current market.
//
// Abstracting this is what lets the same strategy run against a simulated fill
// model in backtest and a real broker in production — only this implementation
// changes.
class ExecutionSimulator {
public:
    virtual ~ExecutionSimulator() = default;

    // Attempt to execute `order` given the current `market` bar. Returns the
    // resulting fill, or nullopt if it doesn't execute (e.g. a limit order that
    // isn't marketable, or no volume to fill against).
    virtual std::optional<FillEvent> execute(const OrderEvent& order, const Bar& market) = 0;
};

}  // namespace bt
