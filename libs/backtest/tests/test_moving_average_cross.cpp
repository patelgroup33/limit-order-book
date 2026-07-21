#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "backtest/data/csv_data_handler.hpp"
#include "backtest/engine.hpp"
#include "backtest/strategy/moving_average_cross.hpp"

using namespace bt;

namespace {

Bar flat_bar(Timestamp t, double price) {
    return Bar{t, price, price, price, price, 100.0};  // degenerate OHLC = price
}

// A close series (fast=2, slow=3) engineered to cross up at t=5 and down at t=8.
const std::vector<double> kCloses = {13, 12, 11, 12, 13, 14, 13, 12, 11};

std::vector<SignalEvent> run_over_closes(MovingAverageCross& strat,
                                         const std::vector<double>& closes) {
    std::vector<SignalEvent> out;
    for (std::size_t i = 0; i < closes.size(); ++i) {
        if (auto s = strat.on_bar(flat_bar(static_cast<Timestamp>(i + 1), closes[i]))) {
            out.push_back(*s);
        }
    }
    return out;
}

// Wires a strategy into the engine and records the signals it emits.
struct StrategyHandler : EventHandler {
    Strategy& strategy;
    std::vector<SignalEvent> signals;
    explicit StrategyHandler(Strategy& s) : strategy(s) {}
    void on_market(const MarketEvent& e, EventQueue& q) override {
        if (auto s = strategy.on_bar(e.bar)) {
            signals.push_back(*s);
            q.push(*s, s->timestamp);  // where the portfolio would pick it up
        }
    }
};

}  // namespace

TEST(MovingAverageCross, RejectsInvalidWindows) {
    EXPECT_THROW(MovingAverageCross(0, 5), std::invalid_argument);
    EXPECT_THROW(MovingAverageCross(3, 3), std::invalid_argument);
    EXPECT_THROW(MovingAverageCross(5, 2), std::invalid_argument);
}

TEST(MovingAverageCross, NoSignalDuringWarmup) {
    MovingAverageCross strat(2, 3);
    EXPECT_FALSE(strat.on_bar(flat_bar(1, 10)).has_value());
    EXPECT_FALSE(strat.on_bar(flat_bar(2, 10)).has_value());  // still < slow window
}

TEST(MovingAverageCross, FiresOnGoldenAndDeathCross) {
    MovingAverageCross strat(2, 3);
    const auto signals = run_over_closes(strat, kCloses);

    ASSERT_EQ(signals.size(), 2u);
    EXPECT_EQ(signals[0].type, SignalType::Long);
    EXPECT_EQ(signals[0].timestamp, 5);
    EXPECT_EQ(signals[1].type, SignalType::Short);
    EXPECT_EQ(signals[1].timestamp, 8);
}

TEST(MovingAverageCross, ResetReproducesSameSignals) {
    MovingAverageCross strat(2, 3);
    const auto first = run_over_closes(strat, kCloses);
    strat.reset();
    const auto second = run_over_closes(strat, kCloses);

    ASSERT_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        EXPECT_EQ(first[i].timestamp, second[i].timestamp);
        EXPECT_EQ(first[i].type, second[i].type);
    }
}

TEST(MovingAverageCross, NameReportsWindows) {
    EXPECT_EQ(MovingAverageCross(10, 30).name(), "MA Cross (10/30)");
}

TEST(MovingAverageCross, FlowsThroughTheEngine) {
    // Build a CSV of the same close series and run it through the full loop.
    std::string csv;
    for (std::size_t i = 0; i < kCloses.size(); ++i) {
        const std::string p = std::to_string(kCloses[i]);
        csv += std::to_string(i + 1) + "," + p + "," + p + "," + p + "," + p + ",100\n";
    }
    CsvOptions opts;
    opts.has_header = false;
    auto data = CsvDataHandler::from_text(csv, opts);

    MovingAverageCross strat(2, 3);
    StrategyHandler handler(strat);
    BacktestEngine engine(data, handler);
    engine.run();

    ASSERT_EQ(handler.signals.size(), 2u);
    EXPECT_EQ(handler.signals[0].type, SignalType::Long);
    EXPECT_EQ(handler.signals[0].timestamp, 5);
    EXPECT_EQ(handler.signals[1].type, SignalType::Short);
    EXPECT_EQ(handler.signals[1].timestamp, 8);
}
