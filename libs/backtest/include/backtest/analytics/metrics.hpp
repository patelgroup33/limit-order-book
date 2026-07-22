#pragma once

#include <cstddef>
#include <vector>

#include "backtest/portfolio/portfolio.hpp"  // EquityPoint

namespace bt {

struct MetricsConfig {
    // Periods per year, used to annualize. 252 for daily bars; for intraday use
    // e.g. 252 * bars_per_day.
    double periods_per_year = 252.0;
    double risk_free_rate = 0.0;  // annual, e.g. 0.02 for 2%
};

// Everything the analytics layer computes from an equity curve plus the list of
// closed-trade PnLs.
struct PerformanceReport {
    // Return-based (from the equity curve)
    double total_return = 0.0;
    double cagr = 0.0;                 // geometric annual growth
    double annualized_return = 0.0;    // arithmetic: mean period return * periods/yr
    double annualized_volatility = 0.0;
    double sharpe_ratio = 0.0;
    double sortino_ratio = 0.0;
    double max_drawdown = 0.0;         // positive fraction, e.g. 0.25 == 25%

    // Trade-based (from the trade PnLs)
    std::size_t num_trades = 0;
    std::size_t num_wins = 0;
    std::size_t num_losses = 0;
    double win_rate = 0.0;
    double profit_factor = 0.0;        // gross profit / gross loss (inf if no losses)
    double avg_win = 0.0;
    double avg_loss = 0.0;             // negative
    double largest_win = 0.0;
    double largest_loss = 0.0;         // negative
    double total_pnl = 0.0;
};

// Compute the full report. Returns zeros for degenerate inputs (empty/one-point
// curve, no trades) rather than NaNs.
[[nodiscard]] PerformanceReport compute_metrics(const std::vector<EquityPoint>& equity_curve,
                                                const std::vector<double>& trade_pnls,
                                                MetricsConfig cfg = {});

}  // namespace bt
