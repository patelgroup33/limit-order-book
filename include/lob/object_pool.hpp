#pragma once

#include <cstddef>
#include <cstring>
#include <deque>

/// @file object_pool.hpp
/// @brief A growing pool of stably-addressed objects with O(1) recycle.

namespace lob {

/// @brief Hands out reusable, stably-addressed `T` objects.
///
/// Purpose: keep the order book's hot path (add / cancel) free of `malloc` and
/// `free`. Cancelling an order returns its node here; the next add reuses it.
///
/// The free-list is *intrusive*: when a slot is released, the pool stores the
/// link to the next free slot **inside that slot's own storage**. So the pool
/// needs no side container for the free-list, and `release` never allocates
/// (hence `noexcept`). This is the classic pool-allocator trick and the reason
/// `T` must be at least pointer-sized.
///
/// Stability comes from `std::deque`, which never relocates existing elements
/// when it grows -- a `T*` handed out stays valid for the pool's lifetime.
///
/// @warning `acquire` returns storage whose contents are *unspecified* (its
/// bytes may hold a stale object or a recycled free-list link). The caller must
/// fully initialise every field before use.
template <typename T>
class ObjectPool {
    static_assert(sizeof(T) >= sizeof(void*),
                  "ObjectPool threads its free-list link through freed slots, "
                  "so T must be at least pointer-sized.");

public:
    ObjectPool() = default;
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = default;
    ObjectPool& operator=(ObjectPool&&) = default;
    ~ObjectPool() = default;

    /// @brief Returns storage for one `T`, recycling a released slot if one is
    ///        available, otherwise growing the pool by one.
    [[nodiscard]] T* acquire() {
        if (free_head_ != nullptr) {
            T* recycled = static_cast<T*>(free_head_);
            // Pop: read the next-free link out of the slot we're handing back.
            std::memcpy(&free_head_, recycled, sizeof(void*));
            --free_count_;
            return recycled;
        }
        return &storage_.emplace_back();  // deque growth keeps addresses stable
    }

    /// @brief Marks a previously-acquired slot as reusable. Never allocates.
    /// @pre `object` came from this pool and is no longer referenced anywhere.
    void release(T* object) noexcept {
        // Push: stash the current head inside the slot, then point head at it.
        std::memcpy(object, &free_head_, sizeof(void*));
        free_head_ = object;
        ++free_count_;
    }

    /// @brief Total objects ever created (high-water mark of concurrent use).
    [[nodiscard]] std::size_t capacity() const noexcept { return storage_.size(); }

    /// @brief Objects currently handed out (acquired but not yet released).
    [[nodiscard]] std::size_t in_use() const noexcept {
        return storage_.size() - free_count_;
    }

private:
    std::deque<T> storage_;         ///< Owns the objects; addresses are stable.
    void*         free_head_ = nullptr;  ///< Head of the intrusive free-list.
    std::size_t   free_count_ = 0;       ///< Number of released (reusable) slots.
};

}  // namespace lob
