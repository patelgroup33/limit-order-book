#include <gtest/gtest.h>

#include <type_traits>
#include <unordered_map>

#include "lob/types.hpp"

using namespace lob;

// ---------------------------------------------------------------------------
// Zero-overhead guarantee: each strong type must be exactly as big as, and as
// trivial as, the integer it wraps. If any of these fail, the abstraction is
// costing us something at runtime.
// ---------------------------------------------------------------------------
static_assert(sizeof(Price) == sizeof(std::int64_t));
static_assert(sizeof(Quantity) == sizeof(std::uint64_t));
static_assert(sizeof(OrderId) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<Price>);
static_assert(std::is_trivially_copyable_v<Quantity>);
static_assert(std::is_trivially_copyable_v<OrderId>);

// Construction from a raw integer must be explicit: `Price p = 100;` should not
// compile. We assert the type trait rather than the (non-)compilation.
static_assert(!std::is_convertible_v<std::int64_t, Price>);
static_assert(std::is_constructible_v<Price, std::int64_t>);

TEST(Price, StoresAndComparesTicks) {
    constexpr Price a{100};
    constexpr Price b{101};
    EXPECT_EQ(a.value(), 100);
    EXPECT_LT(a, b);
    EXPECT_EQ(a, Price{100});
}

TEST(Price, SupportsNegativeValues) {
    // Negative prices are legal (e.g. April 2020 crude oil).
    constexpr Price neg{-3700};
    EXPECT_LT(neg, Price{0});
}

TEST(Quantity, Arithmetic) {
    Quantity q{10};
    q -= Quantity{3};
    EXPECT_EQ(q.value(), 7u);
    q += Quantity{5};
    EXPECT_EQ(q.value(), 12u);
    EXPECT_EQ((Quantity{4} + Quantity{6}).value(), 10u);
}

TEST(Quantity, IsZero) {
    EXPECT_TRUE(Quantity{0}.is_zero());
    EXPECT_FALSE(Quantity{1}.is_zero());
}

TEST(OrderId, EqualityAndHashing) {
    EXPECT_EQ(OrderId{42}, OrderId{42});
    EXPECT_NE(OrderId{42}, OrderId{43});

    // The whole point of the hash specialization: OrderId keys a hash map.
    std::unordered_map<OrderId, int> book_index;
    book_index[OrderId{7}] = 1;
    book_index[OrderId{7}] = 2;  // same key, overwrites
    EXPECT_EQ(book_index.size(), 1u);
    EXPECT_EQ(book_index.at(OrderId{7}), 2);
}

TEST(Sequence, Increments) {
    Sequence s{0};
    ++s;
    ++s;
    EXPECT_EQ(s.value(), 2u);
    EXPECT_LT(Sequence{1}, Sequence{2});
}

TEST(Side, OppositeFlips) {
    EXPECT_EQ(opposite(Side::Buy), Side::Sell);
    EXPECT_EQ(opposite(Side::Sell), Side::Buy);
    // Compile-time too.
    static_assert(opposite(Side::Buy) == Side::Sell);
}
