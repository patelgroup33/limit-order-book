#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <unordered_map>

#include "lob/object_pool.hpp"
#include "lob/order.hpp"
#include "lob/price_ladder.hpp"
#include "lob/price_level.hpp"
#include "lob/types.hpp"

/// @file order_book.hpp
/// @brief The two-sided order book, backed by direct-indexed price ladders (M7).

namespace lob {

/// @brief Holds all resting orders on both sides of a single instrument.
///
/// Since M7 the price levels live in a @ref PriceLadder per side instead of a
/// `std::map`. The public interface is unchanged -- that is the whole point of
/// having isolated the storage behind this class: the container swap is
/// behaviour-preserving, proven by the unchanged test suite.
///
/// Complexity (P = number of distinct active price levels on a side):
///   * add_limit_order : O(1) amortized -- index into the ladder, list append
///   * cancel          : O(1) avg       -- hash the id, unlink the node
///   * best_bid/ask    : O(1)           -- cached best index
///   * match (per fill): O(1)           -- plus O(gap) when a best level empties
class OrderBook {
public:
    OrderBook() = default;
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    /// @brief Rests a fresh limit order in the book.
    /// @return false if the quantity is zero or an order with this id already
    ///         exists (rejected); true otherwise.
    bool add_limit_order(const Order& order);

    /// @brief Cancels a resting order by id. @return false if not resting.
    bool cancel(OrderId id);

    /// @brief Reduces a resting order's quantity in place, keeping time
    ///        priority. @return false if unknown id or not a strict reduction.
    bool reduce_order_quantity(OrderId id, Quantity new_remaining);

    /// @brief Matches an incoming order against the opposite side using
    ///        price-time priority, mutating `incoming.remaining` as it fills.
    ///        Invokes `on_fill(resting_id, exec_price, fill_qty)` per execution.
    ///        Does not rest the leftover (the engine's job).
    /// @tparam OnFill callable as `void(OrderId, Price, Quantity)`.
    template <typename OnFill>
    void match(Order& incoming, OnFill&& on_fill) {
        // A market order crosses any opposing level; a limit order crosses only
        // prices at least as good as its limit.
        const bool is_market = incoming.type == OrderType::Market;
        if (incoming.side == Side::Buy) {
            match_against(asks_, incoming,
                          [&](Price ask) { return is_market || ask <= incoming.price; },
                          on_fill);
        } else {
            match_against(bids_, incoming,
                          [&](Price bid) { return is_market || bid >= incoming.price; },
                          on_fill);
        }
    }

    // ---- Observers --------------------------------------------------------
    [[nodiscard]] bool contains(OrderId id) const { return index_.contains(id); }
    [[nodiscard]] std::size_t order_count() const noexcept { return index_.size(); }
    [[nodiscard]] bool empty() const noexcept { return index_.empty(); }

    /// @brief Read-only view of a resting order, or nullptr if not resting.
    /// @warning Valid only until the order is cancelled/filled.
    [[nodiscard]] const Order* find_order(OrderId id) const;

    [[nodiscard]] std::optional<Price> best_bid() const { return bids_.best_price(); }
    [[nodiscard]] std::optional<Price> best_ask() const { return asks_.best_price(); }

    /// @brief Aggregate resting volume at a price/side (0 if no such level).
    [[nodiscard]] Quantity volume_at(Side side, Price price) const;

private:
    /// @brief Core matching loop against one side of the book.
    template <typename Crosses, typename OnFill>
    void match_against(PriceLadder& opposite, Order& incoming, Crosses crosses, OnFill& on_fill) {
        while (!incoming.remaining.is_zero() && !opposite.empty()) {
            PriceLevel* level = opposite.best_level();
            const Price exec_price = level->price();

            if (!crosses(exec_price)) {
                break;  // best opposite price no longer crosses; done
            }

            // Consume this level in strict FIFO order.
            while (!incoming.remaining.is_zero() && !level->empty()) {
                OrderNode* resting = level->front();
                const Quantity fill = std::min(incoming.remaining, resting->order.remaining);

                on_fill(resting->order.id, exec_price, fill);
                incoming.remaining -= fill;

                if (fill < resting->order.remaining) {
                    // Resting order partially filled: stays at front with less size.
                    resting->order.remaining -= fill;
                    level->reduce_volume(fill);
                } else {
                    // Resting order fully filled: unlink() also drops its volume.
                    level->unlink(resting);
                    index_.erase(resting->order.id);
                    pool_.release(resting);
                }
            }

            if (level->empty()) {
                opposite.advance_best();  // move BBO to the next occupied level
            }
        }
    }

    PriceLadder bids_{true};   ///< bid side: best = highest price
    PriceLadder asks_{false};  ///< ask side: best = lowest price
    std::unordered_map<OrderId, OrderNode*> index_;  ///< id -> node, for O(1) cancel
    ObjectPool<OrderNode> pool_;                     ///< recycles node storage
};

}  // namespace lob
