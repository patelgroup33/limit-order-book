#include "backtest/portfolio/position.hpp"

#include <algorithm>
#include <cmath>

namespace bt {

void Position::apply_fill(Side side, double quantity, double price) {
    const double signed_qty = (side == Side::Buy) ? quantity : -quantity;

    // Opening from flat, or adding in the same direction: blend the average cost.
    if (qty_ == 0.0 || (qty_ > 0.0) == (signed_qty > 0.0)) {
        const double new_qty = qty_ + signed_qty;
        avg_cost_ = (avg_cost_ * std::fabs(qty_) + price * std::fabs(signed_qty)) / std::fabs(new_qty);
        qty_ = new_qty;
        return;
    }

    // Opposite direction: some or all of the position is being closed. Book PnL
    // on the closed amount at the current average cost.
    const double closing = std::min(std::fabs(qty_), std::fabs(signed_qty));
    const double direction = (qty_ > 0.0) ? 1.0 : -1.0;  // long: gain if price up; short: reverse
    realized_pnl_ += direction * (price - avg_cost_) * closing;

    const double new_qty = qty_ + signed_qty;
    if (new_qty == 0.0) {
        qty_ = 0.0;
        avg_cost_ = 0.0;
    } else if ((new_qty > 0.0) == (qty_ > 0.0)) {
        qty_ = new_qty;  // reduced but still on the same side; average cost unchanged
    } else {
        qty_ = new_qty;  // flipped: the remainder opens a fresh position at this price
        avg_cost_ = price;
    }
}

}  // namespace bt
