#pragma once

namespace bt {

// The standard std::visit helper: bundle several lambdas into one callable so a
// visit reads like a switch over the variant's alternatives.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace bt
