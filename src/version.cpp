#include "lob/version.hpp"

namespace lob {

std::string_view version() noexcept {
    // A single translation-unit-local literal. Returning a string_view avoids
    // any allocation and keeps the function noexcept.
    return "0.1.0";
}

}  // namespace lob
