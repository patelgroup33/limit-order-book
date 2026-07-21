#pragma once

#include <optional>
#include <string>

#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"

namespace bt {

// A trading strategy: sees bars in time order and emits at most one signal per
// bar (single instrument). Concrete strategies derive from this and plug into
// the engine without the engine knowing anything about them — the strategy only
// ever sees the current bar, never the future.
class Strategy {
public:
    virtual ~Strategy() = default;

    // React to the next bar. Returns a signal if one fired, otherwise nullopt.
    [[nodiscard]] virtual std::optional<SignalEvent> on_bar(const Bar& bar) = 0;

    // Drop internal state so the strategy can be reused for another run.
    virtual void reset() {}

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace bt
