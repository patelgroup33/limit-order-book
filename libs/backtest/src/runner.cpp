#include "backtest/runner.hpp"

#include <cmath>

namespace bt {

BacktestRunner::BacktestRunner(Strategy& strategy, Portfolio& portfolio, RiskManager& risk,
                               ExecutionSimulator& execution, RunnerConfig cfg)
    : strategy_(strategy), portfolio_(portfolio), risk_(risk), execution_(execution), cfg_(cfg) {}

void BacktestRunner::on_market(const MarketEvent& e, EventQueue& q) {
    current_bar_ = e.bar;
    have_bar_ = true;
    portfolio_.mark(e.bar.timestamp, e.bar.close);  // record an equity-curve point
    if (auto signal = strategy_.on_bar(e.bar)) {
        q.push(*signal, signal->timestamp);
    }
}

void BacktestRunner::on_signal(const SignalEvent& e, EventQueue& q) {
    const double price = current_bar_.close;
    if (price <= 0.0) {
        return;
    }
    const double equity = portfolio_.equity(price);
    const double current = portfolio_.position().quantity();

    // Size the signal into a target position: deploy `allocation` of equity, long
    // or short; Exit targets flat.
    double target = 0.0;
    if (e.type == SignalType::Long) {
        target = std::floor(equity * cfg_.allocation / price);
    } else if (e.type == SignalType::Short) {
        target = -std::floor(equity * cfg_.allocation / price);
    }

    const double delta = target - current;
    if (delta == 0.0) {
        return;
    }

    OrderEvent order{e.timestamp, delta > 0.0 ? Side::Buy : Side::Sell, OrderType::Market,
                     std::fabs(delta), 0.0};

    const RiskResult approved = risk_.check(order, current, price, equity);
    if (approved.decision == RiskDecision::Reject || approved.quantity <= 0.0) {
        return;
    }
    order.quantity = approved.quantity;
    q.push(order, order.timestamp);
}

void BacktestRunner::on_order(const OrderEvent& e, EventQueue& q) {
    if (!have_bar_) {
        return;
    }
    if (auto fill = execution_.execute(e, current_bar_)) {
        q.push(*fill, fill->timestamp);
    }
}

void BacktestRunner::on_fill(const FillEvent& e) {
    fills_.push_back(e);
    const double before = portfolio_.realized_pnl();
    portfolio_.on_fill(e);
    const double realized = portfolio_.realized_pnl() - before;
    if (realized != 0.0) {
        trades_.push_back(TradeRecord{e.timestamp, realized});
    }
}

std::vector<double> BacktestRunner::trade_pnls() const {
    std::vector<double> pnls;
    pnls.reserve(trades_.size());
    for (const auto& trade : trades_) {
        pnls.push_back(trade.pnl);
    }
    return pnls;
}

}  // namespace bt
