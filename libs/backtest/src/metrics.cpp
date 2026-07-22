#include "backtest/analytics/metrics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace bt {
namespace {

double mean_of(const std::vector<double>& v) {
    if (v.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const double x : v) {
        sum += x;
    }
    return sum / static_cast<double>(v.size());
}

}  // namespace

PerformanceReport compute_metrics(const std::vector<EquityPoint>& curve,
                                  const std::vector<double>& trade_pnls, MetricsConfig cfg) {
    PerformanceReport r;
    const double ppy = cfg.periods_per_year;
    const double rf_per = (ppy > 0.0) ? cfg.risk_free_rate / ppy : 0.0;

    // ---------- return-based metrics ----------
    if (curve.size() >= 2) {
        std::vector<double> returns;
        returns.reserve(curve.size() - 1);
        for (std::size_t i = 1; i < curve.size(); ++i) {
            const double prev = curve[i - 1].equity;
            if (prev != 0.0) {
                returns.push_back((curve[i].equity - prev) / prev);
            }
        }

        const double e_start = curve.front().equity;
        const double e_end = curve.back().equity;
        if (e_start != 0.0) {
            r.total_return = (e_end - e_start) / e_start;
        }

        const std::size_t n = returns.size();
        if (n >= 1) {
            const double mu = mean_of(returns);
            r.annualized_return = mu * ppy;

            double var_sum = 0.0;
            double down_sum = 0.0;  // squared negative excess returns (for Sortino)
            for (const double x : returns) {
                var_sum += (x - mu) * (x - mu);
                const double downside = std::min(0.0, x - rf_per);
                down_sum += downside * downside;
            }
            const double denom = (n >= 2) ? static_cast<double>(n - 1) : 1.0;  // sample
            const double sd = std::sqrt(var_sum / denom);
            const double dd = std::sqrt(down_sum / denom);
            const double sqrt_ppy = std::sqrt(ppy);

            r.annualized_volatility = sd * sqrt_ppy;
            if (sd > 0.0) {
                r.sharpe_ratio = (mu - rf_per) / sd * sqrt_ppy;
            }
            if (dd > 0.0) {
                r.sortino_ratio = (mu - rf_per) / dd * sqrt_ppy;
            }

            const double years = static_cast<double>(n) / ppy;
            if (e_start > 0.0 && e_end > 0.0 && years > 0.0) {
                r.cagr = std::pow(e_end / e_start, 1.0 / years) - 1.0;
            }
        }

        // Max drawdown: largest peak-to-trough drop as a fraction of the peak.
        double peak = curve.front().equity;
        for (const auto& point : curve) {
            peak = std::max(peak, point.equity);
            if (peak > 0.0) {
                r.max_drawdown = std::max(r.max_drawdown, (peak - point.equity) / peak);
            }
        }
    }

    // ---------- trade-based metrics ----------
    r.num_trades = trade_pnls.size();
    double gross_profit = 0.0;
    double gross_loss = 0.0;
    for (const double pnl : trade_pnls) {
        r.total_pnl += pnl;
        if (pnl > 0.0) {
            ++r.num_wins;
            gross_profit += pnl;
            r.largest_win = std::max(r.largest_win, pnl);
        } else if (pnl < 0.0) {
            ++r.num_losses;
            gross_loss += -pnl;
            r.largest_loss = std::min(r.largest_loss, pnl);
        }
        // pnl == 0 is a scratch trade: neither win nor loss.
    }
    if (r.num_trades > 0) {
        r.win_rate = static_cast<double>(r.num_wins) / static_cast<double>(r.num_trades);
    }
    if (r.num_wins > 0) {
        r.avg_win = gross_profit / static_cast<double>(r.num_wins);
    }
    if (r.num_losses > 0) {
        r.avg_loss = -gross_loss / static_cast<double>(r.num_losses);
    }
    if (gross_loss > 0.0) {
        r.profit_factor = gross_profit / gross_loss;
    } else if (gross_profit > 0.0) {
        r.profit_factor = std::numeric_limits<double>::infinity();
    }

    return r;
}

}  // namespace bt
