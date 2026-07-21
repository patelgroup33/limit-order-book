#include "backtest/risk/risk_manager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace bt {
namespace {

RiskResult decide(double allowed, double requested) {
    if (allowed <= 0.0) {
        return {RiskDecision::Reject, 0.0};
    }
    if (allowed < requested) {
        return {RiskDecision::Reduce, allowed};
    }
    return {RiskDecision::Accept, requested};
}

}  // namespace

RiskResult RiskManager::check(const OrderEvent& order, double current_position, double price,
                              double equity) const {
    const double requested = order.quantity;
    const double sign = (order.side == Side::Buy) ? 1.0 : -1.0;

    // Daily-loss kill-switch: once the day's loss is too big, block anything that
    // adds risk and allow only orders that reduce toward flat.
    if (limits_.max_daily_loss > 0.0 && (day_start_equity_ - equity) >= limits_.max_daily_loss) {
        if (sign * current_position >= 0.0) {
            return {RiskDecision::Reject, 0.0};  // same direction (or opening) -> more risk
        }
        return decide(std::min(requested, std::fabs(current_position)), requested);
    }

    // The tightest position cap implied by the size and exposure limits.
    double max_pos = std::numeric_limits<double>::infinity();
    if (limits_.max_position_size > 0.0) {
        max_pos = std::min(max_pos, limits_.max_position_size);
    }
    if (limits_.max_exposure > 0.0 && price > 0.0) {
        max_pos = std::min(max_pos, limits_.max_exposure / price);
    }

    double allowed = requested;
    if (std::isfinite(max_pos)) {
        const double abs_pos = std::fabs(current_position);
        // An order opposite the position first closes it (room = cap + |pos|); an
        // order in the same direction only has room up to the cap.
        const double room = (sign * current_position < 0.0) ? (max_pos + abs_pos)
                                                            : std::max(0.0, max_pos - abs_pos);
        allowed = std::min(allowed, room);
    }

    if (limits_.max_order_size > 0.0) {
        allowed = std::min(allowed, limits_.max_order_size);
    }

    return decide(allowed, requested);
}

}  // namespace bt
