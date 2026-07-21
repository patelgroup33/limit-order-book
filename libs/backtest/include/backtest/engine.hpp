#pragma once

#include <chrono>
#include <optional>
#include <thread>
#include <variant>

#include "backtest/core/bar.hpp"
#include "backtest/data/data_handler.hpp"
#include "backtest/event/event.hpp"
#include "backtest/event/event_queue.hpp"
#include "backtest/util/overloaded.hpp"

namespace bt {

// Hooks the engine dispatches events to. Later modules (strategy, portfolio,
// execution) implement these; each hook may schedule follow-on events through
// the queue it's handed (market -> signal -> order -> fill). Defaults are no-ops
// so a partial wiring still compiles and runs.
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void on_market(const MarketEvent&, EventQueue&) {}
    virtual void on_signal(const SignalEvent&, EventQueue&) {}
    virtual void on_order(const OrderEvent&, EventQueue&) {}
    virtual void on_fill(const FillEvent&) {}
};

struct EngineConfig {
    // 0 = run as fast as possible (normal backtesting). > 0 paces playback in
    // real time, treating timestamps as seconds: 1 = real time, 2 = double speed.
    // Real-time pacing is only for watching a run; it never affects results.
    double replay_speed = 0.0;
};

// The replay loop. For each bar in time order it emits a MarketEvent, then
// drains every queued event whose time has arrived. Events scheduled in the
// future (e.g. a latency-delayed fill) wait for a bar to reach their time.
class BacktestEngine {
public:
    BacktestEngine(DataHandler& data, EventHandler& handler, EngineConfig cfg = {})
        : data_(data), handler_(handler), cfg_(cfg) {}

    void run() {
        while (std::optional<Bar> bar = data_.next()) {
            const Timestamp now = bar->timestamp;
            pace(now);
            queue_.push(MarketEvent{*bar}, now);
            while (!queue_.empty() && queue_.next_time() <= now) {
                dispatch(queue_.pop());
            }
        }
        // Flush anything still scheduled (e.g. a fill queued after the last bar).
        while (!queue_.empty()) {
            dispatch(queue_.pop());
        }
    }

private:
    void dispatch(Event event) {
        std::visit(overloaded{
                       [&](const MarketEvent& e) { handler_.on_market(e, queue_); },
                       [&](const SignalEvent& e) { handler_.on_signal(e, queue_); },
                       [&](const OrderEvent& e) { handler_.on_order(e, queue_); },
                       [&](const FillEvent& e) { handler_.on_fill(e); },
                   },
                   event);
    }

    void pace(Timestamp now) {
        if (cfg_.replay_speed <= 0.0) {
            return;  // unbounded — the normal backtest path
        }
        if (have_prev_) {
            const double seconds = static_cast<double>(now - prev_time_) / cfg_.replay_speed;
            if (seconds > 0.0) {
                std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
            }
        }
        prev_time_ = now;
        have_prev_ = true;
    }

    DataHandler&  data_;
    EventHandler& handler_;
    EngineConfig  cfg_;
    EventQueue    queue_;
    Timestamp     prev_time_{0};
    bool          have_prev_{false};
};

}  // namespace bt
