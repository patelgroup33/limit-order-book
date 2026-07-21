#pragma once

#include <cstddef>
#include <optional>

#include "backtest/core/bar.hpp"

namespace bt {

// A source of market data, consumed one bar at a time in time order.
//
// This is the seam that keeps the backtest loop honest: it only ever sees the
// "next" bar, never the whole future. The same interface can later back a live
// feed instead of a file, so the engine loop doesn't change between backtest and
// production.
class DataHandler {
public:
    virtual ~DataHandler() = default;

    // Next bar in time order, or nullopt once the data is exhausted.
    virtual std::optional<Bar> next() = 0;

    [[nodiscard]] virtual bool has_next() const = 0;

    // Total bars available (handy for progress bars and benchmarks).
    [[nodiscard]] virtual std::size_t size() const = 0;
};

}  // namespace bt
