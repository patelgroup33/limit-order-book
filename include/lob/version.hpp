#pragma once

#include <string_view>

/// @file version.hpp
/// @brief Library version information.
///
/// This header exists mostly so Milestone 0 has *something* real to compile,
/// link, and test end-to-end. Every subsequent milestone adds real domain
/// types alongside it.

/// @namespace lob
/// @brief Root namespace for the Limit Order Book engine.
namespace lob {

/// Semantic-version components, usable in `constexpr` contexts and by the build.
inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

/// @brief Returns the library version as a "MAJOR.MINOR.PATCH" string.
/// @return A view into a static string; valid for the program's lifetime.
[[nodiscard]] std::string_view version() noexcept;

}  // namespace lob
