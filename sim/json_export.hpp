#pragma once

#include <ostream>

#include "order_flow_simulator.hpp"

/// @file json_export.hpp
/// @brief Serializes a SimResult to the compact JSON schema the viewer replays.

namespace lobsim {

/// @brief Writes a SimResult as JSON to @c os.
///
/// Schema (levels and trades use positional arrays to keep the file small):
/// @code
/// { "meta": { "instrument", "price_scale", "depth", "frames", "start_mid" },
///   "frames": [ { "t", "last",
///                 "bids": [[price,size,orders], ...],   // best first
///                 "asks": [[price,size,orders], ...],
///                 "trades": [[price,size,"B"|"S"], ...] } ] }
/// @endcode
///
/// The serializer is hand-written on purpose: the schema is small and fixed, so
/// pulling in a JSON dependency would not earn its keep.
inline void write_simulation_json(std::ostream& os, const SimResult& result) {
    const auto levels = [&os](const std::vector<lob::LevelView>& side) {
        os << '[';
        for (std::size_t i = 0; i < side.size(); ++i) {
            const lob::LevelView& lv = side[i];
            if (i != 0) {
                os << ',';
            }
            os << '[' << lv.price.value() << ',' << lv.volume.value() << ','
               << lv.order_count << ']';
        }
        os << ']';
    };

    os << "{\"meta\":{"
       << "\"instrument\":\"SIM\","
       << "\"price_scale\":" << result.price_scale << ','
       << "\"depth\":" << result.depth << ','
       << "\"frames\":" << result.frames.size() << ','
       << "\"start_mid\":" << result.start_mid
       << "},\"frames\":[";

    for (std::size_t f = 0; f < result.frames.size(); ++f) {
        const Frame& frame = result.frames[f];
        if (f != 0) {
            os << ',';
        }
        os << "{\"t\":" << frame.t << ",\"last\":" << frame.last << ",\"bids\":";
        levels(frame.book.bids);
        os << ",\"asks\":";
        levels(frame.book.asks);
        os << ",\"trades\":[";
        for (std::size_t i = 0; i < frame.trades.size(); ++i) {
            const SimTrade& tr = frame.trades[i];
            if (i != 0) {
                os << ',';
            }
            os << '[' << tr.price << ',' << tr.qty << ",\"" << tr.side << "\"]";
        }
        os << "]}";
    }

    os << "]}";
}

}  // namespace lobsim
