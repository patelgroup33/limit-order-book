#pragma once

#include "lob/types.hpp"

/// @file order.hpp
/// @brief The Order value type.

namespace lob {

/// @brief A single order as tracked by the engine.
///
/// This is deliberately a plain aggregate: no inheritance, no virtual
/// functions, no invariants enforced in a constructor. Reasons:
///   * A vtable pointer would bloat every order and add an indirection on the
///     hot path; there is no polymorphism to justify it.
///   * Being trivially copyable lets the engine move orders around cheaply and
///     lets many of them pack tightly into cache lines.
///
/// Validation (e.g. rejecting a zero-quantity or a market order with a price)
/// lives in the MatchingEngine at the system boundary, not in this data type.
struct Order {
    OrderId   id{};         ///< Unique identifier supplied by the client.
    Side      side{};       ///< Buy or Sell.
    OrderType type{};       ///< Limit or Market.
    Price     price{};      ///< Limit price. Unused/ignored for market orders.
    Quantity  quantity{};   ///< Original submitted quantity (never mutated).
    Quantity  remaining{};  ///< Unfilled quantity; decremented as fills occur.
    Sequence  seq{};        ///< Arrival order, stamped by the engine.

    /// @brief True once every unit has been filled.
    [[nodiscard]] constexpr bool is_filled() const noexcept {
        return remaining.is_zero();
    }
};

}  // namespace lob
