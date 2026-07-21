#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

#include "backtest/core/bar.hpp"
#include "backtest/event/event.hpp"

namespace bt {

// Time-ordered event queue.
//
// Events come out in ascending timestamp order, ties broken by insertion order
// (FIFO). The timestamp is what lets execution schedule a fill in the *future*
// (latency): it sits in the queue until a bar reaches its time. Within a single
// bar everything shares a timestamp, so the FIFO tiebreak preserves the natural
// market -> signal -> order -> fill ordering.
class EventQueue {
public:
    void push(Event event, Timestamp time) {
        heap_.push(Entry{time, next_seq_++, std::move(event)});
    }

    [[nodiscard]] bool empty() const noexcept { return heap_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return heap_.size(); }

    // Time of the next event. Precondition: !empty().
    [[nodiscard]] Timestamp next_time() const { return heap_.top().time; }

    // Remove and return the earliest event.
    [[nodiscard]] Event pop() {
        Event event = heap_.top().event;  // variant copy is cheap (small payloads)
        heap_.pop();
        return event;
    }

private:
    struct Entry {
        Timestamp     time;
        std::uint64_t seq;
        Event         event;
    };

    // priority_queue is a max-heap, so "greater" puts the *earliest* on top.
    struct Later {
        bool operator()(const Entry& a, const Entry& b) const noexcept {
            if (a.time != b.time) return a.time > b.time;
            return a.seq > b.seq;
        }
    };

    std::priority_queue<Entry, std::vector<Entry>, Later> heap_;
    std::uint64_t next_seq_{0};
};

}  // namespace bt
