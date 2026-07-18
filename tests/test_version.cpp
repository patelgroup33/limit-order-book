#include <gtest/gtest.h>

#include "lob/version.hpp"

// Milestone 0 smoke test: proves the library compiles, links, and the test
// harness executes. It is intentionally trivial -- its job is to validate the
// *plumbing*, not any behavior.

TEST(Version, ReportsSemanticVersionString) {
    EXPECT_EQ(lob::version(), "0.1.0");
}

TEST(Version, ComponentsAreConsistent) {
    // Compile-time constants must agree with the runtime string above.
    static_assert(lob::version_major == 0);
    static_assert(lob::version_minor == 1);
    static_assert(lob::version_patch == 0);
    SUCCEED();
}
