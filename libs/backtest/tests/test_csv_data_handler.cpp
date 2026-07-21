#include <gtest/gtest.h>

#include <string_view>

#include "backtest/data/csv_data_handler.hpp"

using namespace bt;

namespace {
constexpr std::string_view kValid =
    "timestamp,open,high,low,close,volume\n"
    "1704067200,100.0,101.5,99.8,101.0,15000\n"
    "1704153600,101.0,102.2,100.5,100.8,18000\n"
    "1704240000,100.8,100.9,98.0,98.5,22000\n";
}  // namespace

TEST(CsvDataHandler, ParsesValidRows) {
    auto h = CsvDataHandler::from_text(kValid);
    ASSERT_EQ(h.size(), 3u);

    const Bar& first = h.bars().front();
    EXPECT_EQ(first.timestamp, 1704067200);
    EXPECT_DOUBLE_EQ(first.open, 100.0);
    EXPECT_DOUBLE_EQ(first.high, 101.5);
    EXPECT_DOUBLE_EQ(first.low, 99.8);
    EXPECT_DOUBLE_EQ(first.close, 101.0);
    EXPECT_DOUBLE_EQ(first.volume, 15000.0);
}

TEST(CsvDataHandler, StreamsInOrderThenNullopt) {
    auto h = CsvDataHandler::from_text(kValid);

    auto a = h.next();
    auto b = h.next();
    auto c = h.next();
    ASSERT_TRUE(a && b && c);
    EXPECT_EQ(a->timestamp, 1704067200);
    EXPECT_EQ(b->timestamp, 1704153600);
    EXPECT_EQ(c->timestamp, 1704240000);

    EXPECT_FALSE(h.has_next());
    EXPECT_FALSE(h.next().has_value());

    h.reset();
    EXPECT_TRUE(h.has_next());
    EXPECT_EQ(h.next()->timestamp, 1704067200);
}

TEST(CsvDataHandler, HandlesNoHeaderAndCrlf) {
    constexpr std::string_view data =
        "1,10,11,9,10.5,100\r\n"
        "2,10.5,12,10,11,120\r\n";
    CsvOptions opts;
    opts.has_header = false;
    auto h = CsvDataHandler::from_text(data, opts);
    EXPECT_EQ(h.size(), 2u);
    EXPECT_DOUBLE_EQ(h.bars()[1].close, 11.0);
}

TEST(CsvDataHandler, HeaderOnlyOrEmptyYieldsNoBars) {
    EXPECT_EQ(CsvDataHandler::from_text("timestamp,open,high,low,close,volume\n").size(), 0u);
    EXPECT_EQ(CsvDataHandler::from_text("").size(), 0u);
}

TEST(CsvDataHandler, RejectsWrongColumnCount) {
    EXPECT_THROW(CsvDataHandler::from_text("timestamp,open,high,low,close,volume\n1,2,3,4,5\n"),
                 std::runtime_error);
}

TEST(CsvDataHandler, RejectsNonNumericField) {
    EXPECT_THROW(
        CsvDataHandler::from_text("timestamp,open,high,low,close,volume\n1,abc,3,4,5,6\n"),
        std::runtime_error);
}

TEST(CsvDataHandler, StrictModeRejectsBadOhlcvRelationships) {
    // high < low
    EXPECT_THROW(
        CsvDataHandler::from_text("timestamp,open,high,low,close,volume\n1,10,9,11,10,100\n"),
        std::runtime_error);
    // negative volume
    EXPECT_THROW(
        CsvDataHandler::from_text("timestamp,open,high,low,close,volume\n1,10,11,9,10,-5\n"),
        std::runtime_error);
}

TEST(CsvDataHandler, LenientModeAcceptsBadRelationships) {
    CsvOptions opts;
    opts.strict = false;
    auto h = CsvDataHandler::from_text(
        "timestamp,open,high,low,close,volume\n1,10,9,11,10,100\n", opts);
    EXPECT_EQ(h.size(), 1u);
}

TEST(CsvDataHandler, RejectsBackwardsTimestamps) {
    EXPECT_THROW(
        CsvDataHandler::from_text(
            "timestamp,open,high,low,close,volume\n2,10,11,9,10,1\n1,10,11,9,10,1\n"),
        std::runtime_error);
}
