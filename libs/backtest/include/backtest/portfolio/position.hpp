#pragma once

#include "backtest/core/enums.hpp"

namespace bt {

// Net position in a single instrument, tracked with average-cost accounting.
//
// Quantity is signed: positive is long, negative is short. Realized PnL is
// booked when a fill reduces or closes the position; unrealized PnL is computed
// against a mark price. The average cost is the cost basis of whatever is
// currently open.
class Position {
public:
    // Apply an execution of `quantity` (> 0) at `price` on the given side.
    void apply_fill(Side side, double quantity, double price);

    [[nodiscard]] double quantity() const noexcept { return qty_; }
    [[nodiscard]] double average_cost() const noexcept { return avg_cost_; }
    [[nodiscard]] double realized_pnl() const noexcept { return realized_pnl_; }
    [[nodiscard]] double market_value(double price) const noexcept { return qty_ * price; }
    [[nodiscard]] double unrealized_pnl(double price) const noexcept {
        return (price - avg_cost_) * qty_;
    }
    [[nodiscard]] bool is_flat() const noexcept { return qty_ == 0.0; }

private:
    double qty_{0.0};
    double avg_cost_{0.0};
    double realized_pnl_{0.0};
};

}  // namespace bt
