#pragma once

#include <utility>
#include <vector>

#include "lob/order.hpp"
#include "lob/order_book.hpp"
#include "lob/order_result.hpp"
#include "lob/trade.hpp"
#include "lob/types.hpp"

/// @file matching_engine.hpp
/// @brief Orchestration layer: sequencing, validation, and event emission.

namespace lob {

/// @brief Drives an OrderBook: assigns sequence numbers, validates commands,
///        runs matching, rests leftovers, and emits Trade events.
///
/// The engine owns the *protocol*; the book owns the *mechanism*. Every event
/// (order arrival, each trade) draws the next value from a single monotonic
/// counter, so a given command stream always produces a byte-identical trade
/// stream -- the determinism property that makes the engine replayable.
class MatchingEngine {
public:
    MatchingEngine() = default;
    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    // ---- Limit orders -----------------------------------------------------

    /// @brief Submits a limit order, streaming each resulting trade to `sink`.
    ///        Any unfilled remainder rests in the book.
    /// @tparam TradeSink callable as `void(const Trade&)`.
    template <typename TradeSink>
    OrderResult submit_limit_order(Order order, TradeSink&& sink) {
        order.type = OrderType::Limit;
        return submit(std::move(order), /*rest_leftover=*/true,
                      std::forward<TradeSink>(sink));
    }

    /// @brief Convenience overload: returns the trades in a vector (allocates).
    std::vector<Trade> submit_limit_order(Order order) {
        std::vector<Trade> trades;
        submit_limit_order(std::move(order),
                           [&trades](const Trade& t) { trades.push_back(t); });
        return trades;
    }

    // ---- Market orders ----------------------------------------------------

    /// @brief Submits a market order: matches at any price until filled or the
    ///        book is exhausted. The unfilled remainder is *not* rested.
    /// @tparam TradeSink callable as `void(const Trade&)`.
    template <typename TradeSink>
    OrderResult submit_market_order(OrderId id, Side side, Quantity qty, TradeSink&& sink) {
        Order order{
            .id = id,
            .side = side,
            .type = OrderType::Market,
            .price = Price{},  // unused for market orders
            .quantity = qty,
            .remaining = qty,
            .seq = Sequence{},
        };
        return submit(order, /*rest_leftover=*/false, std::forward<TradeSink>(sink));
    }

    /// @brief Convenience overload: returns the trades in a vector (allocates).
    std::vector<Trade> submit_market_order(OrderId id, Side side, Quantity qty) {
        std::vector<Trade> trades;
        submit_market_order(id, side, qty,
                            [&trades](const Trade& t) { trades.push_back(t); });
        return trades;
    }

    // ---- Modify / cancel --------------------------------------------------

    /// @brief Modifies a resting order.
    ///
    /// Time-priority rules are enforced here:
    ///   * a *pure quantity reduction* at the same price keeps priority (done
    ///     in place);
    ///   * a price change or size increase loses priority (cancel + re-submit),
    ///     and may immediately match if the new price crosses.
    ///
    /// A `new_qty` of zero is treated as a cancel.
    /// @tparam TradeSink callable as `void(const Trade&)`.
    template <typename TradeSink>
    OrderResult modify(OrderId id, Price new_price, Quantity new_qty, TradeSink&& sink) {
        const Order* current = book_.find_order(id);
        if (current == nullptr) {
            return OrderResult::reject(RejectReason::UnknownId);
        }
        if (new_qty.is_zero()) {
            book_.cancel(id);
            return OrderResult{.accepted = true};
        }

        // Copy what we need *before* any mutation invalidates `current`.
        const Side side = current->side;
        const bool same_price = new_price == current->price;
        const bool is_reduction = new_qty < current->remaining;

        if (same_price && is_reduction) {
            book_.reduce_order_quantity(id, new_qty);  // keeps time priority
            return OrderResult{.accepted = true, .resting = new_qty};
        }

        // Cancel/replace: the order goes to the back of its queue and may cross.
        book_.cancel(id);
        Order replacement{
            .id = id,
            .side = side,
            .type = OrderType::Limit,
            .price = new_price,
            .quantity = new_qty,
            .remaining = new_qty,
            .seq = Sequence{},
        };
        return submit(replacement, /*rest_leftover=*/true, std::forward<TradeSink>(sink));
    }

    /// @brief Convenience overload: returns the trades in a vector (allocates).
    std::vector<Trade> modify(OrderId id, Price new_price, Quantity new_qty) {
        std::vector<Trade> trades;
        modify(id, new_price, new_qty,
               [&trades](const Trade& t) { trades.push_back(t); });
        return trades;
    }

    /// @brief Cancels a resting order by id. @return false if not found.
    bool cancel(OrderId id) { return book_.cancel(id); }

    /// @brief Read-only access to the underlying book (for queries/inspection).
    [[nodiscard]] const OrderBook& book() const noexcept { return book_; }

private:
    /// Shared submit path for limit and market orders. `rest_leftover` controls
    /// whether an unfilled remainder rests (limit) or is dropped (market).
    template <typename TradeSink>
    OrderResult submit(Order order, bool rest_leftover, TradeSink&& sink) {
        if (order.quantity.is_zero()) {
            return OrderResult::reject(RejectReason::ZeroQuantity);
        }
        if (book_.contains(order.id)) {
            return OrderResult::reject(RejectReason::DuplicateId);
        }

        order.seq = next_seq();
        order.remaining = order.quantity;
        const Quantity original = order.quantity;

        book_.match(order, [&](OrderId resting_id, Price price, Quantity qty) {
            sink(Trade{
                .aggressor = order.id,
                .resting = resting_id,
                .price = price,
                .quantity = qty,
                .seq = next_seq(),
            });
        });

        const Quantity leftover = order.remaining;
        if (rest_leftover && !leftover.is_zero()) {
            order.quantity = leftover;  // rest the remainder as an order of that size
            book_.add_limit_order(order);
        }

        return OrderResult{
            .accepted = true,
            .reason = RejectReason::None,
            .filled = original - leftover,
            .resting = rest_leftover ? leftover : Quantity{},
        };
    }

    /// Returns the current sequence value and advances the counter.
    [[nodiscard]] Sequence next_seq() noexcept {
        const Sequence current = seq_;
        ++seq_;
        return current;
    }

    OrderBook book_;
    Sequence  seq_{0};  ///< Single monotonic source of "time" for the engine.
};

}  // namespace lob
