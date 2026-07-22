#pragma once

#include <vector>

#include "backtest/core/bar.hpp"
#include "backtest/core/trade_record.hpp"
#include "backtest/engine.hpp"
#include "backtest/event/event.hpp"
#include "backtest/execution/execution_simulator.hpp"
#include "backtest/portfolio/portfolio.hpp"
#include "backtest/risk/risk_manager.hpp"
#include "backtest/strategy/strategy.hpp"

namespace bt {

struct RunnerConfig {
    double allocation = 0.95;  // fraction of equity deployed when taking a position
};

// The glue: the engine's event handler that connects every component.
//   market  -> mark to market, ask the strategy for a signal
//   signal  -> size it, run it past risk, emit an order
//   order   -> execute it into a fill
//   fill    -> book it in the portfolio and record any realized trade
//
// It records nothing the components don't already own except the closed-trade
// log (the equity curve lives in the portfolio).
class BacktestRunner final : public EventHandler {
public:
    BacktestRunner(Strategy& strategy, Portfolio& portfolio, RiskManager& risk,
                   ExecutionSimulator& execution, RunnerConfig cfg = {});

    void on_market(const MarketEvent& e, EventQueue& q) override;
    void on_signal(const SignalEvent& e, EventQueue& q) override;
    void on_order(const OrderEvent& e, EventQueue& q) override;
    void on_fill(const FillEvent& e) override;

    [[nodiscard]] const std::vector<TradeRecord>& trades() const noexcept { return trades_; }
    [[nodiscard]] std::vector<double> trade_pnls() const;

private:
    Strategy&                strategy_;
    Portfolio&               portfolio_;
    RiskManager&             risk_;
    ExecutionSimulator&      execution_;
    RunnerConfig             cfg_;
    Bar                      current_bar_{};
    bool                     have_bar_{false};
    std::vector<TradeRecord> trades_;
};

}  // namespace bt
