#pragma once

#include <ostream>
#include <vector>

#include "backtest/analytics/metrics.hpp"
#include "backtest/core/trade_record.hpp"
#include "backtest/portfolio/portfolio.hpp"

namespace bt {

// Human-readable performance summary.
void write_summary(std::ostream& os, const PerformanceReport& report);

// CSV exports.
void write_equity_curve_csv(std::ostream& os, const std::vector<EquityPoint>& curve);
void write_trades_csv(std::ostream& os, const std::vector<TradeRecord>& trades);

}  // namespace bt
