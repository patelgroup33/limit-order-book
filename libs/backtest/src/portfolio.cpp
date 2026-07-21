#include "backtest/portfolio/portfolio.hpp"

namespace bt {

Portfolio::Portfolio(double initial_cash) : initial_cash_(initial_cash), cash_(initial_cash) {}

void Portfolio::on_fill(const FillEvent& fill) {
    // Buying spends cash, selling receives it; commission always costs.
    const double cash_flow = (fill.side == Side::Buy ? -1.0 : 1.0) * fill.price * fill.quantity;
    cash_ += cash_flow - fill.commission;
    position_.apply_fill(fill.side, fill.quantity, fill.price);
}

void Portfolio::mark(Timestamp t, double price) {
    curve_.push_back(EquityPoint{t, equity(price)});
}

}  // namespace bt
