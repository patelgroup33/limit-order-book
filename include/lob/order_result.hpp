#pragma once

#include <cstdint>

#include "lob/types.hpp"

/// @file order_result.hpp
/// @brief The outcome the engine reports back for a submitted command.

namespace lob {

/// @brief Why an order was rejected (None when it was accepted).
enum class RejectReason : std::uint8_t {
    None,          ///< Accepted.
    ZeroQuantity,  ///< Order had no quantity.
    DuplicateId,   ///< An order with this id is already live.
    UnknownId,     ///< Modify/cancel referenced an id that isn't resting.
};

/// @brief Summarises what happened to a submitted order.
///
/// Trades are still delivered individually through the sink; this struct is the
/// aggregate acknowledgement the caller (a gateway) needs to reply to a client.
struct OrderResult {
    bool         accepted{false};                 ///< Was the command accepted?
    RejectReason reason{RejectReason::None};      ///< Reject reason, if any.
    Quantity     filled{};                        ///< Total quantity matched.
    Quantity     resting{};                        ///< Quantity left resting in the book.

    /// @brief Convenience factory for a rejection.
    [[nodiscard]] static OrderResult reject(RejectReason why) noexcept {
        return OrderResult{.accepted = false, .reason = why, .filled = {}, .resting = {}};
    }
};

}  // namespace lob
