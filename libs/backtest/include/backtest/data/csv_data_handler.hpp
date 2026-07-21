#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "backtest/core/bar.hpp"
#include "backtest/data/data_handler.hpp"

namespace bt {

// Parsing options for CsvDataHandler. Namespace-scoped (not nested) so it can be
// used as a defaulted `= {}` argument on the handler's own members.
struct CsvOptions {
    bool has_header = true;   // skip the first line
    bool strict = true;       // throw on invalid OHLCV relationships
    char delimiter = ',';
};

// Loads an OHLCV CSV fully into memory, then serves the bars in order.
//
// Expected columns: timestamp,open,high,low,close,volume
//
// The file is parsed once into a contiguous vector, so replay is a linear scan
// of cache-friendly memory. That trades memory for speed — fine up to the point
// the data no longer fits in RAM, where you'd stream or memory-map instead (see
// the notes in the .cpp).
class CsvDataHandler final : public DataHandler {
public:
    // Parse a CSV file. Throws std::runtime_error (with the line number) on a
    // malformed row or, in strict mode, on an invalid OHLCV relationship.
    explicit CsvDataHandler(const std::string& path, CsvOptions opts = {});

    // Parse CSV held in memory. Useful for tests and pipelines that don't have a
    // file on disk.
    [[nodiscard]] static CsvDataHandler from_text(std::string_view csv, CsvOptions opts = {});

    std::optional<Bar> next() override;
    [[nodiscard]] bool has_next() const override { return index_ < bars_.size(); }
    [[nodiscard]] std::size_t size() const override { return bars_.size(); }

    void reset() noexcept { index_ = 0; }

    // Direct access to the loaded bars (contiguous), e.g. for vectorized
    // analysis or benchmarks.
    [[nodiscard]] const std::vector<Bar>& bars() const noexcept { return bars_; }

private:
    CsvDataHandler() = default;  // used by from_text
    void parse(std::string_view content, const CsvOptions& opts);

    std::vector<Bar> bars_;
    std::size_t      index_{0};
};

}  // namespace bt
