#include <gtest/gtest.h>

#include <string>

#include "backtest/data/csv_data_handler.hpp"
#include "backtest/engine.hpp"

using namespace bt;

namespace {

// A handler that drives the full chain and records each event it sees, so we can
// assert the exact order the loop processes things. Each bar produces
// market -> signal -> order -> fill; `latency` delays the fill by that many
// timestamp units (to exercise future-scheduled events).
struct RecordingHandler : EventHandler {
    std::string log;
    Timestamp   latency{0};

    void on_market(const MarketEvent& e, EventQueue& q) override {
        log += 'M';
        q.push(SignalEvent{e.bar.timestamp, SignalType::Long, 1.0}, e.bar.timestamp);
    }
    void on_signal(const SignalEvent& e, EventQueue& q) override {
        log += 'S';
        q.push(OrderEvent{e.timestamp, Side::Buy, OrderType::Market, 1.0, 0.0}, e.timestamp);
    }
    void on_order(const OrderEvent& e, EventQueue& q) override {
        log += 'O';
        const Timestamp fill_time = e.timestamp + latency;
        q.push(FillEvent{fill_time, e.side, e.quantity, 0.0, 0.0}, fill_time);
    }
    void on_fill(const FillEvent&) override { log += 'F'; }
};

// Three valid one-unit-apart bars.
constexpr std::string_view kBars =
    "1,10,11,9,10,100\n"
    "2,10,11,9,10,100\n"
    "3,10,11,9,10,100\n";

CsvDataHandler bars(std::size_t n) {
    CsvOptions opts;
    opts.has_header = false;
    std::string text;
    for (std::size_t i = 0; i < n; ++i) {
        text += std::to_string(i + 1) + ",10,11,9,10,100\n";
    }
    return CsvDataHandler::from_text(text, opts);
}

}  // namespace

TEST(BacktestEngine, ImmediateFillProcessesChainPerBar) {
    auto data = bars(2);
    RecordingHandler handler;  // latency 0
    BacktestEngine engine(data, handler);
    engine.run();
    // Each bar: market, signal, order, fill — all within the same timestamp.
    EXPECT_EQ(handler.log, "MSOFMSOF");
}

TEST(BacktestEngine, LatencyDefersFillToNextBar) {
    auto data = bars(3);
    RecordingHandler handler;
    handler.latency = 1;  // fill lands one timestamp later
    BacktestEngine engine(data, handler);
    engine.run();
    // Bar1: M,S,O (fill queued for t=2). Bar2: the deferred fill fires first,
    // then M,S,O (fill queued for t=3). Bar3 likewise, and the final fill is
    // flushed after the loop.
    EXPECT_EQ(handler.log, "MSOFMSOFMSOF");
}

TEST(BacktestEngine, EmptyDataDoesNothing) {
    auto data = bars(0);
    RecordingHandler handler;
    BacktestEngine engine(data, handler);
    engine.run();
    EXPECT_TRUE(handler.log.empty());
}

TEST(BacktestEngine, DefaultHandlerJustReplaysBars) {
    CsvOptions opts;
    opts.has_header = false;
    auto data = CsvDataHandler::from_text(kBars, opts);
    EventHandler handler;  // all no-ops
    BacktestEngine engine(data, handler);
    engine.run();  // should simply consume all bars without doing anything
    EXPECT_FALSE(data.has_next());
}
