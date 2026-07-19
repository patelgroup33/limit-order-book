#pragma once

#include <cstddef>
#include <vector>

#include "lob/types.hpp"

/// @file book_snapshot.hpp
/// @brief A point-in-time view of the top of the book (market data / L2).

namespace lob {

/// @brief One aggregated price level as seen by an outside observer.
///
/// This is *market data*: it exposes only the aggregate at a price (total size
/// and order count), never individual orders. That is exactly what a real
/// exchange publishes on its L2 feed -- the queue's internals stay private.
struct LevelView {
    Price       price;        ///< The price of this level.
    Quantity    volume;       ///< Aggregate resting size at this price.
    std::size_t order_count;  ///< Number of resting orders at this price.
};

/// @brief The top @c depth levels of each side, best price first.
///
/// `bids` are ordered highest-to-lowest price; `asks` lowest-to-highest -- so
/// `bids.front()` and `asks.front()` are the best bid and ask. A visualizer (or
/// any market-data consumer) renders directly from this.
struct BookSnapshot {
    std::vector<LevelView> bids;  ///< Best (highest) first.
    std::vector<LevelView> asks;  ///< Best (lowest) first.
};

}  // namespace lob
