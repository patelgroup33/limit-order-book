# ---------------------------------------------------------------------------
# Test build. GoogleTest is pulled in reproducibly via FetchContent so the repo
# has no system dependency on a pre-installed gtest: `git clone` + `cmake` just
# works on any machine (and in CI).
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2  # pinned: reproducible builds, no surprise breakage
)

# On Windows, make gtest link the shared CRT to match our targets. Harmless
# elsewhere; keeps the project cross-platform out of the box.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# One test executable for the whole suite. As milestones land we add their
# test files here; a single binary keeps test-discovery fast.
add_executable(lob_tests
    test_version.cpp
    test_types.cpp
    test_order.cpp
    test_price_level.cpp
    test_object_pool.cpp
    test_order_book.cpp
    test_matching_engine.cpp
    test_market_and_modify.cpp
    test_snapshot.cpp
)

target_link_libraries(lob_tests PRIVATE
    lob                 # the code under test
    GTest::gtest_main   # provides main() + the assertion framework
)

# gtest_discover_tests registers each TEST() with CTest at *build* time, so
# `ctest` lists them individually (better than add_test on the whole binary).
gtest_discover_tests(lob_tests)
