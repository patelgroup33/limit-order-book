#pragma once

#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

/// @file types.hpp
/// @brief Strongly-typed domain primitives for the order book.
///
/// The core engine never uses raw `int`/`double` for prices, quantities, or
/// ids. Each concept is its own type, exposing *only* the operations that are
/// meaningful for it. This turns a whole class of bugs (argument swaps, mixing
/// a quantity with a price) into compile errors, at zero runtime cost -- every
/// wrapper is the size of the integer it holds and is trivially copyable.

namespace lob {

// ---------------------------------------------------------------------------
// Price -- a point on an integer "tick" axis.
//
// Prices are integers, never floating point. IEEE-754 can't compare equal
// reliably (0.1 + 0.2 != 0.3), isn't associative (breaks deterministic replay),
// and real exchanges quote in integer ticks anyway. `value()` is a count of
// minimum price increments; conversion to/from dollars happens at the edge of
// the system, never in the matching core.
//
// The representation is *signed*: some markets really do trade at negative
// prices (e.g. WTI crude oil futures settled at -$37 in April 2020).
//
// Price is ordered and comparable but has no arithmetic: you compare prices to
// decide priority and whether orders cross; you don't "add two prices".
// ---------------------------------------------------------------------------
class Price {
public:
    using rep = std::int64_t;

    Price() = default;
    constexpr explicit Price(rep ticks) noexcept : ticks_(ticks) {}

    [[nodiscard]] constexpr rep value() const noexcept { return ticks_; }

    // Defaulted spaceship gives all six relational operators *and* ==, which is
    // exactly what the book's ordered maps and the crossing checks need.
    friend constexpr auto operator<=>(const Price&, const Price&) = default;

private:
    rep ticks_{0};
};

// ---------------------------------------------------------------------------
// Quantity -- a non-negative count of units (shares, contracts, ...).
//
// Unsigned to express "quantities are never negative". Subtraction is guarded
// by an assertion because underflow (filling more than remains) is always a
// logic bug, and on unsigned it would silently wrap to a gigantic number.
// ---------------------------------------------------------------------------
class Quantity {
public:
    using rep = std::uint64_t;

    Quantity() = default;
    constexpr explicit Quantity(rep qty) noexcept : qty_(qty) {}

    [[nodiscard]] constexpr rep value() const noexcept { return qty_; }
    [[nodiscard]] constexpr bool is_zero() const noexcept { return qty_ == 0; }

    constexpr Quantity& operator+=(Quantity other) noexcept {
        qty_ += other.qty_;
        return *this;
    }
    constexpr Quantity& operator-=(Quantity other) noexcept {
        assert(qty_ >= other.qty_ && "Quantity underflow: filled more than available");
        qty_ -= other.qty_;
        return *this;
    }

    friend constexpr Quantity operator+(Quantity a, Quantity b) noexcept { return a += b; }
    friend constexpr Quantity operator-(Quantity a, Quantity b) noexcept { return a -= b; }

    friend constexpr auto operator<=>(const Quantity&, const Quantity&) = default;

private:
    rep qty_{0};
};

// ---------------------------------------------------------------------------
// OrderId -- an opaque, unique identifier for an order.
//
// Note what is *absent*: no arithmetic and no ordering. Adding or subtracting
// order ids is nonsense, and there is no natural "less than" on ids, so those
// operations don't exist and misusing an id won't compile. All OrderId needs is
// equality (are these the same order?) and hashing (to live in a hash map).
// ---------------------------------------------------------------------------
class OrderId {
public:
    using rep = std::uint64_t;

    OrderId() = default;
    constexpr explicit OrderId(rep id) noexcept : id_(id) {}

    [[nodiscard]] constexpr rep value() const noexcept { return id_; }

    constexpr bool operator==(const OrderId&) const = default;

private:
    rep id_{0};
};

// ---------------------------------------------------------------------------
// Sequence -- a monotonically increasing counter the engine stamps on each
// order as it arrives. This, not a wall-clock timestamp, is our source of
// "time" for time priority: clocks can run backwards and two events can share a
// timestamp, but a sequence number is strictly increasing and makes the whole
// engine deterministic and replayable.
// ---------------------------------------------------------------------------
class Sequence {
public:
    using rep = std::uint64_t;

    Sequence() = default;
    constexpr explicit Sequence(rep seq) noexcept : seq_(seq) {}

    [[nodiscard]] constexpr rep value() const noexcept { return seq_; }

    constexpr Sequence& operator++() noexcept {  // pre-increment: next arrival
        ++seq_;
        return *this;
    }

    friend constexpr auto operator<=>(const Sequence&, const Sequence&) = default;

private:
    rep seq_{0};
};

// ---------------------------------------------------------------------------
// Enumerations. Both use a 1-byte underlying type to keep Order compact -- a
// side or type never needs more than a byte, and every saved byte helps pack
// more orders into a cache line.
// ---------------------------------------------------------------------------
enum class Side : std::uint8_t { Buy, Sell };
enum class OrderType : std::uint8_t { Limit, Market };

/// Returns the opposing side. A buy order matches against resting sells and
/// vice versa, so the engine reaches for this constantly.
[[nodiscard]] constexpr Side opposite(Side side) noexcept {
    return side == Side::Buy ? Side::Sell : Side::Buy;
}

}  // namespace lob

// ---------------------------------------------------------------------------
// Hash support so OrderId can key an std::unordered_map (the O(1) order index).
// Specializing std::hash is the idiomatic way to make a user type hashable.
// ---------------------------------------------------------------------------
template <>
struct std::hash<lob::OrderId> {
    [[nodiscard]] std::size_t operator()(lob::OrderId id) const noexcept {
        return std::hash<lob::OrderId::rep>{}(id.value());
    }
};
