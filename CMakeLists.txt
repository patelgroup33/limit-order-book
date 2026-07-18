# ---------------------------------------------------------------------------
# Top-level build script for the LOB (Limit Order Book) project.
#
# Design goals:
#   * Modern, target-based CMake (no global include_directories / link_libraries).
#   * The core lives in a reusable library target `lob`; tests, benchmarks and
#     any future executables link against it. This mirrors how production code
#     is packaged: the matching engine is a library, and the exchange gateway,
#     tests, and tools are all just consumers of it.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(lob
    VERSION 0.1.0
    DESCRIPTION "Production-inspired C++20 Limit Order Book and Matching Engine"
    HOMEPAGE_URL "https://github.com/patelgroup33/limit-order-book"
    LANGUAGES CXX)

# In-source builds pollute the tree with generated files and make `git clean`
# dangerous. Fail fast and force the user into a dedicated build directory.
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not allowed. Run cmake from a separate directory, "
        "e.g. `cmake -S . -B build`.")
endif()

# ---- Language standard ----------------------------------------------------
# We require C++20 with no compiler extensions (-std=c++20, not -std=gnu++20),
# so the code stays portable across GCC/Clang/MSVC.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release so casual `cmake -B build` builds are optimized. Benchmarks
# are meaningless in a Debug build, and this is fundamentally a performance
# project. A developer can still override with -DCMAKE_BUILD_TYPE=Debug.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# Export compile_commands.json so clangd / clang-tidy / IDEs work out of the box.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Options --------------------------------------------------------------
option(LOB_BUILD_TESTS      "Build unit tests"           ON)
option(LOB_BUILD_BENCHMARKS "Build benchmarks"           OFF)
option(LOB_BUILD_DOCS       "Generate Doxygen API docs"  OFF)
option(LOB_ENABLE_WARNINGS  "Enable strict warnings"     ON)

# ---- The core library -----------------------------------------------------
add_library(lob
    src/version.cpp
    src/order_book.cpp
)

# PUBLIC include dir: consumers (tests/benchmarks) inherit it automatically.
# The generator expression keeps the path correct whether we build in-tree or,
# later, install the library.
target_include_directories(lob PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Propagate the C++20 requirement to anything that links `lob`.
target_compile_features(lob PUBLIC cxx_std_20)

# Strict warnings are opt-out, not opt-in: we want to be told about narrowing
# conversions and shadowing early. -Wconversion in particular is invaluable in
# a codebase full of integer prices/quantities.
if(LOB_ENABLE_WARNINGS)
    target_compile_options(lob PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

# ---- Tests ----------------------------------------------------------------
if(LOB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---- Benchmarks -----------------------------------------------------------
if(LOB_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# ---- Documentation --------------------------------------------------------
if(LOB_BUILD_DOCS)
    add_subdirectory(docs)
endif()
