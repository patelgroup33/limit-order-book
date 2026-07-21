# ---------------------------------------------------------------------------
# backtest — the backtesting engine library.
# Built as part of the monorepo; self-contained for now (own types), to be
# unified with libs/core during the restructure.
# ---------------------------------------------------------------------------
add_library(backtest
    src/csv_data_handler.cpp
    src/moving_average_cross.cpp
    src/position.cpp
    src/portfolio.cpp
    src/simulated_execution.cpp
    src/risk_manager.cpp
)

target_include_directories(backtest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(backtest PUBLIC cxx_std_20)

if(LOB_ENABLE_WARNINGS)
    target_compile_options(backtest PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

if(LOB_BUILD_TESTS)
    # GoogleTest is fetched once and shared across the monorepo; FetchContent
    # dedups by name, so declaring it here too is safe.
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)

    add_executable(backtest_tests
        tests/test_csv_data_handler.cpp
        tests/test_event_queue.cpp
        tests/test_backtest_engine.cpp
        tests/test_moving_average_cross.cpp
        tests/test_position.cpp
        tests/test_portfolio.cpp
        tests/test_simulated_execution.cpp
        tests/test_risk_manager.cpp
    )
    target_link_libraries(backtest_tests PRIVATE backtest GTest::gtest_main)
    gtest_discover_tests(backtest_tests)
endif()
