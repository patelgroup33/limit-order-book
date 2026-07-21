#include "backtest/execution/simulated_execution.hpp"

#include <algorithm>

namespace bt {

std::optional<FillEvent> SimulatedExecution::execute(const OrderEvent& order, const Bar& market) {
    const double base = market.close;
    double fill_price = base;

    if (order.type == OrderType::Market) {
        // Slippage moves the price against you: a buy pays up, a sell gets less.
        const double slip = base * cfg_.slippage_bps / 10'000.0;
        fill_price = base + (order.side == Side::Buy ? slip : -slip);
    } else {  // Limit
        const bool marketable = (order.side == Side::Buy) ? base <= order.limit_price
                                                          : base >= order.limit_price;
        if (!marketable) {
            return std::nullopt;  // market hasn't reached the limit
        }
        fill_price = base;  // fills at the prevailing price, which honors the limit
    }

    // Partial fill: optionally cap by how much of the bar's volume we can take.
    double fill_qty = order.quantity;
    if (cfg_.max_volume_fraction > 0.0) {
        fill_qty = std::min(fill_qty, cfg_.max_volume_fraction * market.volume);
    }
    if (fill_qty <= 0.0) {
        return std::nullopt;
    }

    double commission = cfg_.commission_per_share * fill_qty + cfg_.commission_rate * fill_price * fill_qty;
    commission = std::max(commission, cfg_.min_commission);

    const Timestamp fill_time = order.timestamp + cfg_.latency;
    return FillEvent{fill_time, order.side, fill_qty, fill_price, commission};
}

}  // namespace bt
