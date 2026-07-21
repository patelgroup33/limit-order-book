#pragma once

#include <cstdint>

namespace bt {

// Epoch timestamp for a bar. Kept as an integer so ordering and equality are
// exact; the unit (e.g. seconds or nanoseconds since epoch) is a property of the
// data source and stays consistent for the length of a run.
using Timestamp = std::int64_t;

// One OHLCV bar: the open/high/low/close prices and volume over a fixed time
// interval. Plain trivially-copyable value type, so millions of them stream
// through contiguous, cache-friendly memory.
//
// Prices are doubles here, unlike the matching engine's integer ticks. A
// backtester computes returns, Sharpe, drawdown, etc. in floating point anyway,
// and historical prices arrive as decimals — so double is the natural fit. The
// integer-price rule exists for the matcher's exact price-level equality, which
// isn't a concern when replaying bars.
struct Bar {
    Timestamp timestamp{0};
    double    open{0.0};
    double    high{0.0};
    double    low{0.0};
    double    close{0.0};
    double    volume{0.0};
};

}  // namespace bt
