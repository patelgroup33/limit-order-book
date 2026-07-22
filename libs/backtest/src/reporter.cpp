#include "backtest/report/reporter.hpp"

#include <iomanip>

namespace bt {
namespace {

void number(std::ostream& os, const char* label, double value, int precision = 2) {
    os << std::left << std::setw(20) << label << std::right << std::fixed
       << std::setprecision(precision) << value << '\n';
}

void percent(std::ostream& os, const char* label, double fraction) {
    os << std::left << std::setw(20) << label << std::right << std::fixed << std::setprecision(2)
       << (fraction * 100.0) << "%\n";
}

}  // namespace

void write_summary(std::ostream& os, const PerformanceReport& r) {
    os << "=== Performance Summary ===\n";
    percent(os, "Total Return", r.total_return);
    percent(os, "CAGR", r.cagr);
    percent(os, "Annualized Vol", r.annualized_volatility);
    number(os, "Sharpe Ratio", r.sharpe_ratio);
    number(os, "Sortino Ratio", r.sortino_ratio);
    percent(os, "Max Drawdown", r.max_drawdown);
    os << "--- trades ---\n";
    os << std::left << std::setw(20) << "Trades" << std::right << r.num_trades << '\n';
    percent(os, "Win Rate", r.win_rate);
    number(os, "Profit Factor", r.profit_factor);
    number(os, "Avg Win", r.avg_win);
    number(os, "Avg Loss", r.avg_loss);
    number(os, "Total PnL", r.total_pnl);
}

void write_equity_curve_csv(std::ostream& os, const std::vector<EquityPoint>& curve) {
    os << "timestamp,equity\n";
    for (const auto& point : curve) {
        os << point.timestamp << ',' << point.equity << '\n';
    }
}

void write_trades_csv(std::ostream& os, const std::vector<TradeRecord>& trades) {
    os << "timestamp,pnl\n";
    for (const auto& trade : trades) {
        os << trade.timestamp << ',' << trade.pnl << '\n';
    }
}

}  // namespace bt
