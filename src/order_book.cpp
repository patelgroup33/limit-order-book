#include "lob/order_book.hpp"

namespace lob {

bool OrderBook::add_limit_order(const Order& order) {
    if (order.quantity.is_zero()) {
        return false;
    }
    if (index_.contains(order.id)) {
        return false;
    }

    OrderNode* node = pool_.acquire();
    node->order = order;
    node->order.remaining = order.quantity;  // a freshly rested order is unfilled
    node->prev = nullptr;                     // acquire() may return recycled memory;
    node->next = nullptr;                     // clear stale links before linking

    if (order.side == Side::Buy) {
        bids_.add(node);
    } else {
        asks_.add(node);
    }

    index_.emplace(order.id, node);
    return true;
}

bool OrderBook::cancel(OrderId id) {
    const auto it = index_.find(id);
    if (it == index_.end()) {
        return false;
    }

    OrderNode* node = it->second;
    if (node->order.side == Side::Buy) {
        bids_.remove(node);
    } else {
        asks_.remove(node);
    }

    index_.erase(it);
    pool_.release(node);  // recycle the node
    return true;
}

bool OrderBook::reduce_order_quantity(OrderId id, Quantity new_remaining) {
    const auto it = index_.find(id);
    if (it == index_.end()) {
        return false;
    }
    OrderNode* node = it->second;
    const Quantity current = node->order.remaining;
    if (new_remaining >= current) {
        return false;  // not a reduction; caller must cancel/replace instead
    }

    const Quantity delta = current - new_remaining;
    node->order.remaining = new_remaining;
    node->order.quantity = new_remaining;  // keep displayed size consistent

    if (node->order.side == Side::Buy) {
        bids_.reduce(node, delta);
    } else {
        asks_.reduce(node, delta);
    }
    return true;
}

const Order* OrderBook::find_order(OrderId id) const {
    const auto it = index_.find(id);
    return it == index_.end() ? nullptr : &it->second->order;
}

Quantity OrderBook::volume_at(Side side, Price price) const {
    const PriceLadder& ladder = (side == Side::Buy) ? bids_ : asks_;
    const PriceLevel* level = ladder.level_at(price);
    return level != nullptr ? level->total_volume() : Quantity{};
}

}  // namespace lob
