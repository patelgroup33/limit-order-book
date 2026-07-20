#pragma once

#include <cstddef>

#include "lob/order.hpp"
#include "lob/types.hpp"

namespace lob {

// An order plus the prev/next links used to thread it into a PriceLevel's list.
// The links live here rather than in Order so Order stays a plain value type.
// Nodes are owned by the pool, not by the level, and are always referred to by
// pointer once they're in the book (copying a linked node would carry stale
// links).
struct OrderNode {
    Order      order;
    OrderNode* prev{nullptr};
    OrderNode* next{nullptr};
};

// All orders resting at one price, oldest first (FIFO / time priority).
//
// It's an intrusive doubly-linked list: append and unlink are both O(1), and we
// keep a running volume so depth queries don't have to walk the list. The level
// doesn't own the nodes, so it's move-only (copying would leave two levels
// pointing at the same nodes).
class PriceLevel {
public:
    explicit PriceLevel(Price price) noexcept : price_(price) {}

    PriceLevel(const PriceLevel&) = delete;
    PriceLevel& operator=(const PriceLevel&) = delete;
    PriceLevel(PriceLevel&&) noexcept = default;
    PriceLevel& operator=(PriceLevel&&) noexcept = default;
    ~PriceLevel() = default;

    [[nodiscard]] Price price() const noexcept { return price_; }
    [[nodiscard]] Quantity total_volume() const noexcept { return volume_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }

    // Oldest resting order (highest time priority), or null if empty.
    [[nodiscard]] OrderNode* front() const noexcept { return head_; }

    // Append at the tail. node must not already be linked somewhere.
    void push_back(OrderNode* node) noexcept {
        node->prev = tail_;
        node->next = nullptr;
        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;  // was empty: node is head and tail
        }
        tail_ = node;
        ++size_;
        volume_ += node->order.remaining;
    }

    // Remove node from this level. Handles head/tail/middle uniformly.
    void unlink(OrderNode* node) noexcept {
        if (node->prev != nullptr) {
            node->prev->next = node->next;
        } else {
            head_ = node->next;
        }
        if (node->next != nullptr) {
            node->next->prev = node->prev;
        } else {
            tail_ = node->prev;
        }
        node->prev = nullptr;  // clear links so a use-after-unlink null-derefs
        node->next = nullptr;
        --size_;
        volume_ -= node->order.remaining;
    }

    // Called when a resting order is partially filled but stays in the book, so
    // the aggregate volume tracks the reduced size.
    void reduce_volume(Quantity filled) noexcept { volume_ -= filled; }

private:
    Price       price_;
    OrderNode*  head_{nullptr};
    OrderNode*  tail_{nullptr};
    std::size_t size_{0};
    Quantity    volume_{};
};

}  // namespace lob
