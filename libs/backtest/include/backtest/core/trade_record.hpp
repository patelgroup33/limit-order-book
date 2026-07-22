#pragma once

#include "backtest/core/bar.hpp"  // Timestamp

namespace bt {

// A closed (realized) trade outcome: when a fill reduces or closes a position,
// the realized PnL from that close is recorded here.
struct TradeRecord {
    Timestamp timestamp;
    double    pnl;
};

}  // namespace bt
