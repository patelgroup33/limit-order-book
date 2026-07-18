#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "lob/order.hpp"
#include "lob/price_level.hpp"
#include "lob/types.hpp"

/// @file price_ladder.hpp
/// @brief One side of the book as a direct-indexed price ladder (M7).

namespace lob {

/// @brief One side of the book: slot `i` holds all orders at price `base_ + i`.
///
/// This replaces the M3 `std::map<Price, PriceLevel>`. Add, cancel, and
/// best-price access are O(1) (add is amortized O(1) when the covered price
/// band must grow). Levels live in one contiguous `std::vector`, so walking the
/// book during matching is a linear scan of adjacent memory rather than a
/// pointer chase across scattered red-black-tree nodes -- the cache-locality win
/// this milestone is about, and the structure real matching engines use.
///
/// The trade-off is a memory footprint proportional to the width of the price
/// band actually touched (empty in-band levels cost one `PriceLevel` each). The
/// band grows lazily to fit whatever prices arrive; a real venue would size it
/// from the instrument's tick bands up front.
class PriceLadder {
public:
    /// @param is_bid true for the bid side (best = highest price), false for
    ///               the ask side (best = lowest price).
    explicit PriceLadder(bool is_bid) noexcept : is_bid_(is_bid) {}

    /// @brief True if no orders rest on this side.
    [[nodiscard]] bool empty() const noexcept { return best_ < 0; }

    /// @brief The best (most aggressive) price, or nullopt if empty.
    [[nodiscard]] std::optional<Price> best_price() const noexcept {
        if (best_ < 0) {
            return std::nullopt;
        }
        return Price{base_ + best_};
    }

    /// @brief The best level (mutable), or nullptr if empty.
    [[nodiscard]] PriceLevel* best_level() noexcept {
        return best_ < 0 ? nullptr : &slots_[static_cast<std::size_t>(best_)];
    }

    /// @brief Read-only view of the level at `price`, or nullptr if `price` is
    ///        outside the covered band. The returned level may be empty.
    [[nodiscard]] const PriceLevel* level_at(Price price) const noexcept {
        if (slots_.empty()) {
            return nullptr;
        }
        const std::int64_t idx = price.value() - base_;
        if (idx < 0 || idx >= static_cast<std::int64_t>(slots_.size())) {
            return nullptr;
        }
        return &slots_[static_cast<std::size_t>(idx)];
    }

    /// @brief Rests a node at its price (growing the band if needed) and updates
    ///        the best index in O(1).
    void add(OrderNode* node) {
        PriceLevel& level = level_ref(node->order.price);
        level.push_back(node);
        const std::int64_t idx = node->order.price.value() - base_;
        if (best_ < 0 || (is_bid_ ? idx > best_ : idx < best_)) {
            best_ = idx;  // this order sits at a new best price
        }
    }

    /// @brief Unlinks a node; if that empties the current best level, advances
    ///        the best index to the next occupied level.
    void remove(OrderNode* node) noexcept {
        const std::int64_t idx = node->order.price.value() - base_;
        PriceLevel& level = slots_[static_cast<std::size_t>(idx)];
        level.unlink(node);
        if (idx == best_ && level.empty()) {
            advance_best();
        }
    }

    /// @brief Reduces the aggregate volume of a node's (non-emptying) level.
    void reduce(OrderNode* node, Quantity delta) noexcept {
        const std::int64_t idx = node->order.price.value() - base_;
        slots_[static_cast<std::size_t>(idx)].reduce_volume(delta);
    }

    /// @brief Moves the best index to the next occupied level after the current
    ///        best level has been emptied (called by the matcher). Scans toward
    ///        worse prices; O(gap), usually O(1) since prices cluster near BBO.
    void advance_best() noexcept {
        if (best_ < 0) {
            return;
        }
        if (is_bid_) {
            std::int64_t i = best_;
            while (i >= 0 && slots_[static_cast<std::size_t>(i)].empty()) {
                --i;
            }
            best_ = i;  // -1 if nothing remains
        } else {
            const std::int64_t n = static_cast<std::int64_t>(slots_.size());
            std::int64_t i = best_;
            while (i < n && slots_[static_cast<std::size_t>(i)].empty()) {
                ++i;
            }
            best_ = (i < n) ? i : -1;
        }
    }

private:
    /// @brief Returns the level at `price`, growing (and rebasing, for prices
    ///        below the current base) the ladder so `price` is covered. New
    ///        slots are constructed carrying their own price.
    PriceLevel& level_ref(Price price) {
        const std::int64_t pv = price.value();

        if (slots_.empty()) {  // first order establishes the base price
            base_ = pv;
            slots_.emplace_back(Price{pv});
            return slots_.front();
        }

        if (pv < base_) {  // prepend lower-priced slots and lower the base
            const std::int64_t prepend = base_ - pv;
            std::vector<PriceLevel> grown;
            grown.reserve(slots_.size() + static_cast<std::size_t>(prepend));
            for (std::int64_t i = 0; i < prepend; ++i) {
                grown.emplace_back(Price{pv + i});
            }
            for (auto& existing : slots_) {
                grown.push_back(std::move(existing));
            }
            slots_ = std::move(grown);
            base_ = pv;
            if (best_ >= 0) {
                best_ += prepend;  // every existing index shifted up
            }
            return slots_.front();
        }

        const std::int64_t idx = pv - base_;
        for (std::int64_t i = static_cast<std::int64_t>(slots_.size()); i <= idx; ++i) {
            slots_.emplace_back(Price{base_ + i});  // extend upward, prices assigned
        }
        return slots_[static_cast<std::size_t>(idx)];
    }

    std::vector<PriceLevel> slots_;      ///< slot i corresponds to price base_ + i
    std::int64_t            base_ = 0;   ///< price at index 0 (valid once non-empty)
    std::int64_t            best_ = -1;  ///< index of best occupied level, -1 if empty
    bool                    is_bid_;     ///< bid: best = max index; ask: best = min index
};

}  // namespace lob
