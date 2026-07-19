#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "lob/matching_engine.hpp"

/// @file order_flow_simulator.hpp
/// @brief A synthetic market that drives the engine to produce a replayable run.

/// @namespace lobsim
/// @brief The simulation/visualization app (distinct from the `lob` library).
namespace lobsim {

/// @brief A trade as recorded for the visualizer (aggressor side included).
struct SimTrade {
    std::int64_t  price;  ///< Execution price, in ticks.
    std::uint64_t qty;    ///< Traded size.
    char          side;   ///< Aggressor side: 'B' (buy) or 'S' (sell).
};

/// @brief One recorded frame: the book after a command, plus trades it caused.
struct Frame {
    std::size_t          t;       ///< Frame index (our notion of time).
    std::int64_t         last;    ///< Last trade price so far, in ticks.
    lob::BookSnapshot    book;    ///< Top-N depth on both sides.
    std::vector<SimTrade> trades; ///< Trades printed on this step (usually 0-few).
};

/// @brief The whole recorded run plus the metadata the viewer needs.
struct SimResult {
    std::int64_t       start_mid;    ///< Mid price the sim started at, in ticks.
    std::size_t        depth;        ///< Levels per side in each snapshot.
    std::int64_t       price_scale;  ///< Ticks per currency unit (100 -> cents).
    std::vector<Frame> frames;
};

/// @brief Generates a plausible stream of limit/market/cancel commands around a
///        drifting mid price and records a frame per command.
///
/// The generator is seeded, so a given config yields a byte-identical run --
/// the same determinism principle as the engine itself, which makes the demo
/// reproducible.
class OrderFlowSimulator {
public:
    struct Config {
        std::int64_t start_mid = 10000;  ///< e.g. 10000 ticks == 100.00 at scale 100
        std::int64_t price_scale = 100;  ///< display divisor
        std::size_t  depth = 12;         ///< snapshot depth per side
        unsigned     seed = 42;          ///< RNG seed (reproducibility)
        std::size_t  warmup = 60;        ///< passive orders to seed liquidity first
        std::int64_t max_drift = 300;    ///< clamp mid to start_mid +/- this
    };

    explicit OrderFlowSimulator(Config cfg) : cfg_(cfg), rng_(cfg.seed), mid_(cfg.start_mid),
                                              last_price_(cfg.start_mid) {}

    /// @brief Seeds liquidity, then runs @c steps commands, one frame each.
    SimResult run(std::size_t steps) {
        seed_liquidity();

        SimResult result;
        result.start_mid = cfg_.start_mid;
        result.depth = cfg_.depth;
        result.price_scale = cfg_.price_scale;
        result.frames.reserve(steps);

        for (std::size_t t = 0; t < steps; ++t) {
            pending_.clear();
            act();
            drift_mid();
            result.frames.push_back(Frame{
                .t = t,
                .last = last_price_,
                .book = engine_.book().snapshot(cfg_.depth),
                .trades = pending_,
            });
        }
        return result;
    }

private:
    // --- command generation ------------------------------------------------

    // Returns a trade sink that tags each trade with the given aggressor side
    // and updates the running last price. Defined before its callers because it
    // has a deduced (auto) return type.
    auto trade_sink(lob::Side aggressor) {
        const char tag = (aggressor == lob::Side::Buy) ? 'B' : 'S';
        return [this, tag](const lob::Trade& tr) {
            pending_.push_back(SimTrade{tr.price.value(), tr.quantity.value(), tag});
            last_price_ = tr.price.value();
        };
    }

    void act() {
        const double u = unit_(rng_);
        if (u < 0.55) {
            place_limit(coin_side(), /*aggressive=*/false);
        } else if (u < 0.70) {
            place_limit(coin_side(), /*aggressive=*/true);  // likely to cross -> trade
        } else if (u < 0.85) {
            place_market(coin_side());                      // consumes liquidity -> trade
        } else {
            cancel_random();
        }
    }

    void place_limit(lob::Side side, bool aggressive) {
        const std::int64_t price = limit_price(side, aggressive);
        const std::uint64_t qty = rand_qty();
        lob::Order order{
            .id = lob::OrderId{next_id_++},
            .side = side,
            .type = lob::OrderType::Limit,
            .price = lob::Price{price},
            .quantity = lob::Quantity{qty},
            .remaining = lob::Quantity{qty},
            .seq = lob::Sequence{},
        };
        const lob::OrderId id = order.id;
        const lob::OrderResult res = engine_.submit_limit_order(order, trade_sink(side));
        if (res.accepted && res.resting.value() > 0) {
            track(id);  // some size rested; it can be cancelled later
        }
    }

    void place_market(lob::Side side) {
        engine_.submit_market_order(lob::OrderId{next_id_++}, side, lob::Quantity{rand_qty()},
                                    trade_sink(side));
    }

    void cancel_random() {
        if (live_.empty()) {
            return;
        }
        const std::size_t idx = static_cast<std::size_t>(rand_range(0, static_cast<std::int64_t>(live_.size()) - 1));
        engine_.cancel(live_[idx]);          // no-op if already filled; that's fine
        live_[idx] = live_.back();           // swap-erase; order in the vector is irrelevant
        live_.pop_back();
    }

    // --- pricing / sizing --------------------------------------------------

    std::int64_t limit_price(lob::Side side, bool aggressive) {
        // Passive orders rest just off the mid; aggressive ones reach across it.
        const std::int64_t reach = aggressive ? rand_range(0, 3) : -(1 + rand_range(0, 8));
        return side == lob::Side::Buy ? mid_ + reach : mid_ - reach;
    }

    std::uint64_t rand_qty() { return static_cast<std::uint64_t>(rand_range(1, 20)) * 5u; }

    void drift_mid() {
        if (unit_(rng_) < 0.15) {  // slow random walk keeps the book visibly moving
            mid_ += (unit_(rng_) < 0.5) ? -1 : 1;
            const std::int64_t lo = cfg_.start_mid - cfg_.max_drift;
            const std::int64_t hi = cfg_.start_mid + cfg_.max_drift;
            mid_ = std::min(std::max(mid_, lo), hi);
        }
    }

    // --- bookkeeping -------------------------------------------------------

    void seed_liquidity() {
        for (std::size_t i = 0; i < cfg_.warmup; ++i) {
            place_limit(coin_side(), /*aggressive=*/false);
        }
    }

    void track(lob::OrderId id) { live_.push_back(id); }

    lob::Side coin_side() { return unit_(rng_) < 0.5 ? lob::Side::Buy : lob::Side::Sell; }

    std::int64_t rand_range(std::int64_t lo, std::int64_t hi) {
        return std::uniform_int_distribution<std::int64_t>{lo, hi}(rng_);
    }

    Config                     cfg_;
    lob::MatchingEngine        engine_;
    std::mt19937               rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
    std::int64_t               mid_;
    std::int64_t               last_price_;
    std::uint64_t              next_id_ = 1;
    std::vector<lob::OrderId>  live_;      ///< resting ids eligible for cancel
    std::vector<SimTrade>      pending_;   ///< trades produced by the current step
};

}  // namespace lobsim
