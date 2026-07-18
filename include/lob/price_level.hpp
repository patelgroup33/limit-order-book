#pragma once

#include <cstddef>

#include "lob/order.hpp"
#include "lob/types.hpp"

/// @file price_level.hpp
/// @brief A single price level: a FIFO queue of orders resting at one price.

namespace lob {

/// @brief An Order plus the intrusive hooks that let it live in a PriceLevel.
///
/// The `prev`/`next` pointers are *list bookkeeping*, deliberately kept out of
/// the `Order` value type so `Order` stays trivially copyable. A node is owned
/// by whoever allocated it (an object pool, in Milestone 3), never by the
/// PriceLevel that links it.
///
/// @warning A node must not be copied while it is linked into a level: the copy
/// would carry stale `prev`/`next` pointers. Nodes are referred to by pointer,
/// never by value, once they enter the book.
struct OrderNode {
    Order      order;            ///< The order itself.
    OrderNode* prev{nullptr};    ///< Previous (older) node, or null if head.
    OrderNode* next{nullptr};    ///< Next (newer) node, or null if tail.
};

/// @brief All orders resting at a single price, in strict time (FIFO) priority.
///
/// Implemented as an intrusive doubly-linked list so that appending a new order
/// and cancelling an arbitrary order are both O(1). The level maintains the
/// aggregate resting volume incrementally, so querying depth is also O(1).
///
/// The class is non-owning: it manipulates links but never allocates or frees
/// `OrderNode`s. It is move-only, because copying would alias the same external
/// nodes from two levels.
class PriceLevel {
public:
    /// @brief Constructs an empty level at the given price.
    explicit PriceLevel(Price price) noexcept : price_(price) {}

    // Move-only: the level holds non-owning pointers into external storage;
    // copying would create two levels aliasing the same nodes.
    PriceLevel(const PriceLevel&) = delete;
    PriceLevel& operator=(const PriceLevel&) = delete;
    PriceLevel(PriceLevel&&) noexcept = default;
    PriceLevel& operator=(PriceLevel&&) noexcept = default;
    ~PriceLevel() = default;

    // ---- Observers (all O(1)) --------------------------------------------
    [[nodiscard]] Price price() const noexcept { return price_; }
    [[nodiscard]] Quantity total_volume() const noexcept { return volume_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }

    /// @brief The oldest resting order -- the one with highest time priority.
    /// @return Pointer to the front node, or nullptr if the level is empty.
    [[nodiscard]] OrderNode* front() const noexcept { return head_; }

    // ---- Mutators (all O(1)) ---------------------------------------------

    /// @brief Appends a node at the tail (it becomes the newest / lowest
    ///        priority order at this price). Adds its remaining qty to volume.
    /// @pre `node` is not currently linked into any level.
    void push_back(OrderNode* node) noexcept {
        node->prev = tail_;
        node->next = nullptr;
        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;  // list was empty; this node is now head *and* tail
        }
        tail_ = node;
        ++size_;
        volume_ += node->order.remaining;
    }

    /// @brief Unlinks an arbitrary node from this level in O(1) and subtracts
    ///        its remaining qty from volume.
    /// @pre `node` is currently linked into *this* level.
    void unlink(OrderNode* node) noexcept {
        if (node->prev != nullptr) {
            node->prev->next = node->next;
        } else {
            head_ = node->next;  // node was the head
        }
        if (node->next != nullptr) {
            node->next->prev = node->prev;
        } else {
            tail_ = node->prev;  // node was the tail
        }
        // Null the node's own links so a stale pointer can't be followed later.
        node->prev = nullptr;
        node->next = nullptr;
        --size_;
        volume_ -= node->order.remaining;
    }

    /// @brief Records that a resting order at this level was partially filled,
    ///        keeping the aggregate volume in sync. The engine calls this when
    ///        it reduces an order's `remaining` without removing the order.
    void reduce_volume(Quantity filled) noexcept { volume_ -= filled; }

private:
    Price       price_;            ///< The price this level represents.
    OrderNode*  head_{nullptr};    ///< Oldest order (front of the FIFO queue).
    OrderNode*  tail_{nullptr};    ///< Newest order (back of the FIFO queue).
    std::size_t size_{0};          ///< Number of resting orders.
    Quantity    volume_{};         ///< Sum of `remaining` across all orders.
};

}  // namespace lob
