#pragma once

#include "lob/types.hpp"

/// @file trade.hpp
/// @brief The Trade value type: the record of a single execution.

namespace lob {

/// @brief An immutable record that two orders matched for some quantity.
///
/// A single incoming order can generate *many* trades (it may sweep several
/// resting orders), so trades are the engine's primary output events.
///
/// The execution price is always the *resting* order's price, never the
/// aggressor's. The resting order was there first and set the terms; the
/// aggressor accepted them. This is standard price-improvement behavior: a buy
/// order willing to pay 101 that hits a resting sell at 100 trades at 100.
struct Trade {
    OrderId  aggressor{};  ///< The incoming order that crossed the spread.
    OrderId  resting{};    ///< The passive order that was resting in the book.
    Price    price{};      ///< Execution price = the resting order's price.
    Quantity quantity{};   ///< Matched size for this execution.
    Sequence seq{};        ///< Engine sequence at which this trade occurred.
};

}  // namespace lob
