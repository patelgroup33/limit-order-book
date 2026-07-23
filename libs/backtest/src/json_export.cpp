#include "backtest/report/json_export.hpp"

#include <cmath>
#include <cstddef>
#include <iomanip>

namespace bt {

void write_backtest_json(std::ostream& os, const std::string& strategy_name, double initial_cash,
                         const std::vector<Bar>& bars, const std::vector<EquityPoint>& equity,
                         const std::vector<FillEvent>& fills, const PerformanceReport& r) {
    os << std::setprecision(10);

    os << "{\"meta\":{\"strategy\":\"" << strategy_name << "\",\"initial_cash\":" << initial_cash
       << "},";

    // JSON has no infinity; report an "undefined" profit factor as -1.
    const double profit_factor = std::isinf(r.profit_factor) ? -1.0 : r.profit_factor;
    os << "\"metrics\":{"
       << "\"total_return\":" << r.total_return << ','
       << "\"cagr\":" << r.cagr << ','
       << "\"annualized_vol\":" << r.annualized_volatility << ','
       << "\"sharpe\":" << r.sharpe_ratio << ','
       << "\"sortino\":" << r.sortino_ratio << ','
       << "\"max_drawdown\":" << r.max_drawdown << ','
       << "\"num_trades\":" << r.num_trades << ','
       << "\"win_rate\":" << r.win_rate << ','
       << "\"profit_factor\":" << profit_factor << ','
       << "\"total_pnl\":" << r.total_pnl << "},";

    os << "\"bars\":[";
    for (std::size_t i = 0; i < bars.size(); ++i) {
        if (i != 0) os << ',';
        os << '[' << bars[i].timestamp << ',' << bars[i].close << ']';
    }
    os << "],";

    os << "\"equity\":[";
    for (std::size_t i = 0; i < equity.size(); ++i) {
        if (i != 0) os << ',';
        os << '[' << equity[i].timestamp << ',' << equity[i].equity << ']';
    }
    os << "],";

    os << "\"fills\":[";
    for (std::size_t i = 0; i < fills.size(); ++i) {
        if (i != 0) os << ',';
        const FillEvent& f = fills[i];
        os << '[' << f.timestamp << ",\"" << (f.side == Side::Buy ? 'B' : 'S') << "\","
           << f.price << ',' << f.quantity << ']';
    }
    os << "]}";
}

}  // namespace bt
