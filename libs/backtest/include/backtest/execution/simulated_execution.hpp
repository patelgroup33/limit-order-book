#pragma once

#include <optional>

#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"
#include "backtest/execution/execution_simulator.hpp"

namespace bt {

// Cost/fill model for the simulator. All effects are off by default so a plain
// config gives frictionless fills; turn each on to make the backtest realistic.
struct ExecutionConfig {
    double    slippage_bps = 0.0;          // slippage as basis points of price (market orders)
    double    commission_per_share = 0.0;  // fixed cost per unit filled
    double    commission_rate = 0.0;       // proportional cost (fraction of notional)
    double    min_commission = 0.0;        // per-order commission floor
    double    max_volume_fraction = 0.0;   // if > 0, cap the fill at this fraction of bar volume
    Timestamp latency = 0;                 // fill lands this many timestamp units after the order
};

// A realistic-enough bar execution model:
//   * market orders fill at the bar's close, moved by slippage (buys pay up,
//     sells receive less);
//   * limit orders fill at the market price only if it has reached their limit,
//     otherwise they don't execute;
//   * fills can be partial, capped by a fraction of the bar's volume;
//   * commission is charged; and fills can be delayed by a fixed latency.
class SimulatedExecution final : public ExecutionSimulator {
public:
    explicit SimulatedExecution(ExecutionConfig cfg = {}) : cfg_(cfg) {}

    std::optional<FillEvent> execute(const OrderEvent& order, const Bar& market) override;

private:
    ExecutionConfig cfg_;
};

}  // namespace bt
