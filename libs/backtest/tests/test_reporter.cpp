#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "backtest/report/reporter.hpp"

using namespace bt;

TEST(Reporter, SummaryContainsKeyMetrics) {
    PerformanceReport r;
    r.total_return = 0.21;
    r.sharpe_ratio = 1.25;
    r.max_drawdown = 0.123;
    r.num_trades = 5;
    r.win_rate = 0.6;

    std::ostringstream os;
    write_summary(os, r);
    const std::string out = os.str();

    EXPECT_NE(out.find("Performance Summary"), std::string::npos);
    EXPECT_NE(out.find("21.00%"), std::string::npos);  // total return
    EXPECT_NE(out.find("1.25"), std::string::npos);    // sharpe
    EXPECT_NE(out.find("12.30%"), std::string::npos);  // max drawdown
    EXPECT_NE(out.find("60.00%"), std::string::npos);  // win rate
}

TEST(Reporter, EquityCurveCsvHasHeaderAndRows) {
    std::vector<EquityPoint> curve{{1, 100.0}, {2, 105.5}};
    std::ostringstream os;
    write_equity_curve_csv(os, curve);
    const std::string out = os.str();
    EXPECT_NE(out.find("timestamp,equity"), std::string::npos);
    EXPECT_NE(out.find("1,100"), std::string::npos);
    EXPECT_NE(out.find("2,105.5"), std::string::npos);
}

TEST(Reporter, TradesCsvHasHeaderAndRows) {
    std::vector<TradeRecord> trades{{10, 250.0}, {20, -75.0}};
    std::ostringstream os;
    write_trades_csv(os, trades);
    const std::string out = os.str();
    EXPECT_NE(out.find("timestamp,pnl"), std::string::npos);
    EXPECT_NE(out.find("10,250"), std::string::npos);
    EXPECT_NE(out.find("20,-75"), std::string::npos);
}
