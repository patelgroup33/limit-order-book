#pragma once

#include <vector>

#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"
#include "backtest/portfolio/position.hpp"

namespace bt {

struct EquityPoint {
    Timestamp timestamp;
    double    equity;
};

// Account state as fills arrive and the market moves: cash, the position,
// realized/unrealized PnL, total equity, and the equity curve over time.
//
// Cash accounting is simple and mark-to-market: a buy spends cash, a sell adds
// it, commission always subtracts, and equity = cash + position market value.
// (No margin/leverage yet — buying power is just available cash.)
class Portfolio {
public:
    explicit Portfolio(double initial_cash);

    // Apply an execution: move cash and update the position.
    void on_fill(const FillEvent& fill);

    // Mark the account to `price` at time `t` and append an equity-curve point.
    void mark(Timestamp t, double price);

    [[nodiscard]] double cash() const noexcept { return cash_; }
    [[nodiscard]] double buying_power() const noexcept { return cash_; }
    [[nodiscard]] double realized_pnl() const noexcept { return position_.realized_pnl(); }
    [[nodiscard]] double unrealized_pnl(double price) const noexcept {
        return position_.unrealized_pnl(price);
    }
    [[nodiscard]] double equity(double price) const noexcept {
        return cash_ + position_.market_value(price);
    }
    [[nodiscard]] double initial_cash() const noexcept { return initial_cash_; }

    [[nodiscard]] const Position& position() const noexcept { return position_; }
    [[nodiscard]] const std::vector<EquityPoint>& equity_curve() const noexcept { return curve_; }

private:
    double                   initial_cash_;
    double                   cash_;
    Position                 position_;
    std::vector<EquityPoint> curve_;
};

}  // namespace bt
