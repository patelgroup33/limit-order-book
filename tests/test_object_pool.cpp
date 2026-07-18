#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "lob/object_pool.hpp"

using namespace lob;

// The pool threads its free-list through freed slots, so T must be at least
// pointer-sized. Use a 64-bit element (int is only 4 bytes on most platforms).
using Cell = std::int64_t;

TEST(ObjectPool, AcquireGrowsAndAddressesAreStable) {
    ObjectPool<Cell> pool;
    std::vector<Cell*> handed_out;
    for (Cell i = 0; i < 1000; ++i) {
        Cell* p = pool.acquire();
        *p = i;
        handed_out.push_back(p);
    }
    EXPECT_EQ(pool.capacity(), 1000u);
    EXPECT_EQ(pool.in_use(), 1000u);

    // Every earlier pointer must still be valid and hold its value: growth
    // never relocated existing objects.
    for (Cell i = 0; i < 1000; ++i) {
        EXPECT_EQ(*handed_out[static_cast<std::size_t>(i)], i);
    }
}

TEST(ObjectPool, ReleaseRecyclesMemory) {
    ObjectPool<Cell> pool;
    Cell* a = pool.acquire();
    Cell* b = pool.acquire();
    EXPECT_EQ(pool.in_use(), 2u);
    EXPECT_EQ(pool.capacity(), 2u);

    pool.release(a);
    EXPECT_EQ(pool.in_use(), 1u);

    // Next acquire should reuse the released slot, not grow the pool.
    Cell* c = pool.acquire();
    EXPECT_EQ(c, a);                  // same memory recycled
    EXPECT_EQ(pool.capacity(), 2u);   // no growth
    EXPECT_EQ(pool.in_use(), 2u);
    EXPECT_NE(b, c);
}
