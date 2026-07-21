#include "backtest/data/csv_data_handler.hpp"

#include <array>
#include <charconv>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace bt {
namespace {

[[noreturn]] void fail(std::size_t line, const std::string& msg) {
    throw std::runtime_error("CSV parse error on line " + std::to_string(line) + ": " + msg);
}

std::string_view trim(std::string_view sv) {
    const auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (!sv.empty() && is_space(sv.front())) sv.remove_prefix(1);
    while (!sv.empty() && is_space(sv.back())) sv.remove_suffix(1);
    return sv;
}

// Parse a whole field with std::from_chars — locale-independent, no allocation,
// and it fails loudly on trailing garbage (ptr must reach the end).
template <typename T>
T parse_field(std::string_view sv, std::size_t line, const char* what) {
    sv = trim(sv);
    T value{};
    const char* first = sv.data();
    const char* last = sv.data() + sv.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        fail(line, std::string("bad ") + what + " value '" + std::string(sv) + "'");
    }
    return value;
}

}  // namespace

CsvDataHandler::CsvDataHandler(const std::string& path, CsvOptions opts) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("could not open CSV file: " + path);
    }
    const std::streamsize sz = in.tellg();
    in.seekg(0);
    std::string content;
    content.resize(static_cast<std::size_t>(sz < 0 ? 0 : sz));
    if (sz > 0 && !in.read(content.data(), sz)) {
        throw std::runtime_error("failed to read CSV file: " + path);
    }
    parse(content, opts);
}

CsvDataHandler CsvDataHandler::from_text(std::string_view csv, CsvOptions opts) {
    CsvDataHandler handler;
    handler.parse(csv, opts);
    return handler;
}

void CsvDataHandler::parse(std::string_view content, const CsvOptions& opts) {
    bars_.reserve(content.size() / 32 + 1);  // rough guess to limit reallocs

    std::size_t line_no = 0;
    std::size_t pos = 0;
    Timestamp last_ts = std::numeric_limits<Timestamp>::min();
    bool have_prev = false;

    while (pos < content.size()) {
        const std::size_t eol = content.find('\n', pos);
        std::string_view line = (eol == std::string_view::npos)
                                    ? content.substr(pos)
                                    : content.substr(pos, eol - pos);
        pos = (eol == std::string_view::npos) ? content.size() : eol + 1;
        ++line_no;

        line = trim(line);
        if (line.empty()) continue;                       // blank line
        if (line_no == 1 && opts.has_header) continue;    // header row

        // Split into exactly six fields; the last one takes the remainder so a
        // stray trailing delimiter or extra column is caught by from_chars.
        std::array<std::string_view, 6> fields;
        std::size_t fpos = 0;
        for (int i = 0; i < 5; ++i) {
            const std::size_t d = line.find(opts.delimiter, fpos);
            if (d == std::string_view::npos) fail(line_no, "expected 6 columns");
            fields[static_cast<std::size_t>(i)] = line.substr(fpos, d - fpos);
            fpos = d + 1;
        }
        fields[5] = line.substr(fpos);

        Bar bar;
        bar.timestamp = parse_field<Timestamp>(fields[0], line_no, "timestamp");
        bar.open = parse_field<double>(fields[1], line_no, "open");
        bar.high = parse_field<double>(fields[2], line_no, "high");
        bar.low = parse_field<double>(fields[3], line_no, "low");
        bar.close = parse_field<double>(fields[4], line_no, "close");
        bar.volume = parse_field<double>(fields[5], line_no, "volume");

        if (opts.strict) {
            if (bar.high < bar.low) fail(line_no, "high < low");
            if (bar.high < bar.open || bar.high < bar.close) fail(line_no, "high below open/close");
            if (bar.low > bar.open || bar.low > bar.close) fail(line_no, "low above open/close");
            if (bar.volume < 0.0) fail(line_no, "negative volume");
        }
        if (have_prev && bar.timestamp < last_ts) {
            fail(line_no, "timestamp goes backwards (data must be sorted by time)");
        }
        last_ts = bar.timestamp;
        have_prev = true;

        bars_.push_back(bar);
    }
}

std::optional<Bar> CsvDataHandler::next() {
    if (index_ >= bars_.size()) return std::nullopt;
    return bars_[index_++];
}

}  // namespace bt
