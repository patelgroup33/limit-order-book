#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "backtest/core/bar.hpp"
#include "backtest/data/data_handler.hpp"

namespace bt {

// A DataHandler backed by an in-memory vector of bars. Useful for programmatic
// data (generated series, tests, benchmarks) with no file involved.
class VectorDataHandler final : public DataHandler {
public:
    explicit VectorDataHandler(std::vector<Bar> bars) : bars_(std::move(bars)) {}

    std::optional<Bar> next() override {
        if (index_ >= bars_.size()) {
            return std::nullopt;
        }
        return bars_[index_++];
    }
    [[nodiscard]] bool has_next() const override { return index_ < bars_.size(); }
    [[nodiscard]] std::size_t size() const override { return bars_.size(); }

    void reset() noexcept { index_ = 0; }

private:
    std::vector<Bar> bars_;
    std::size_t      index_{0};
};

}  // namespace bt
