# ---------------------------------------------------------------------------
# Benchmark build. Wired up now but only compiled when -DLOB_BUILD_BENCHMARKS=ON.
# Real benchmarks arrive in Milestone 6, once there is an engine to measure.
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.0
)

# Google Benchmark tries to build its own tests by default; we don't want them.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

add_executable(lob_benchmarks
    bench_order_book.cpp
)

target_link_libraries(lob_benchmarks PRIVATE
    lob
    benchmark::benchmark_main
)
