#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "backtest/analytics/metrics.hpp"
#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"
#include "backtest/portfolio/portfolio.hpp"

namespace bt {

// Writes a backtest run as JSON for the web visualizer: metadata, the metrics,
// the price series, the equity curve, and the fills (as chart markers).
void write_backtest_json(std::ostream& os, const std::string& strategy_name, double initial_cash,
                         const std::vector<Bar>& bars, const std::vector<EquityPoint>& equity,
                         const std::vector<FillEvent>& fills, const PerformanceReport& report);

}  // namespace bt
