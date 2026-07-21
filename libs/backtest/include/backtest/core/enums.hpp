#pragma once

#include <cstdint>

namespace bt {

enum class Side : std::uint8_t { Buy, Sell };
enum class OrderType : std::uint8_t { Market, Limit };

// What a strategy wants to do, before it's turned into a sized order.
enum class SignalType : std::uint8_t { Long, Short, Exit };

[[nodiscard]] constexpr Side opposite(Side side) noexcept {
    return side == Side::Buy ? Side::Sell : Side::Buy;
}

}  // namespace bt
