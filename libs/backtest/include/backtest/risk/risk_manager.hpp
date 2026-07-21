#pragma once

#include "backtest/event/event.hpp"

namespace bt {

// Risk limits. Each is disabled when set to 0 (unlimited).
struct RiskLimits {
    double max_position_size = 0.0;  // cap on |position| in units
    double max_exposure = 0.0;       // cap on |position * price| notional
    double max_order_size = 0.0;     // cap on a single order's quantity
    double max_daily_loss = 0.0;     // halt new risk once the day's loss exceeds this
};

enum class RiskDecision { Accept, Reduce, Reject };

struct RiskResult {
    RiskDecision decision;
    double       quantity;  // approved quantity (<= requested; 0 if rejected)
};

// Vets proposed orders against the limits given the current position and equity.
// It never sends orders itself — it only approves, shrinks, or blocks them, so
// the sizing (portfolio) and the gating (risk) stay separate concerns.
class RiskManager {
public:
    explicit RiskManager(RiskLimits limits = {}) : limits_(limits) {}

    // Record the equity at the start of a trading day; the daily-loss limit is
    // measured against this baseline. The engine calls it on each new day.
    void on_new_day(double equity) noexcept { day_start_equity_ = equity; }

    // Evaluate `order` given the current signed position, mark price, and equity.
    [[nodiscard]] RiskResult check(const OrderEvent& order, double current_position, double price,
                                   double equity) const;

private:
    RiskLimits limits_;
    double     day_start_equity_{0.0};
};

}  // namespace bt
