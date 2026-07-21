#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>

#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"
#include "backtest/strategy/strategy.hpp"

namespace bt {

// Dual moving-average crossover on the close price. Goes Long when the fast MA
// crosses above the slow MA (a "golden cross") and Short when it crosses below
// (a "death cross").
//
// Both averages are kept as rolling sums over a sliding window, so each bar is
// O(1) — no re-summing the window every step, which is what keeps it fast on
// millions of bars.
class MovingAverageCross final : public Strategy {
public:
    // Requires 0 < fast_window < slow_window; throws std::invalid_argument
    // otherwise.
    MovingAverageCross(std::size_t fast_window, std::size_t slow_window);

    std::optional<SignalEvent> on_bar(const Bar& bar) override;
    void reset() override;
    [[nodiscard]] std::string name() const override;

private:
    std::size_t        fast_;
    std::size_t        slow_;
    std::deque<double> window_;      // the last `slow_` closes
    double             fast_sum_{0.0};
    double             slow_sum_{0.0};
    int                prev_sign_{0};  // sign of (fastMA - slowMA) on the last full window
};

}  // namespace bt
