// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file heap_pool.hpp
 * @brief Fixed-capacity object pool with pointer stability for VM temporaries.
 *
 * Replaces unbounded std::deque growth in BytecodeVM heap pools.
 * Elements are pointer-stable once acquired (never relocated), which is
 * critical for NaN-boxed values that store raw pointers to pool entries.
 *
 * Usage:
 *   HeapPool<std::string> strings(1024);
 *   std::string& s = strings.Acquire();
 *   s = "hello";
 *   // ... use &s as a stable pointer ...
 *   strings.Reset();  // reuse slots next frame
 *
 * @date 04/04/2026
 * @author Coela
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <vector>

namespace koilo {

/// Fixed-capacity object pool with pointer-stable elements.
/// Slots are pre-allocated in contiguous chunks. Acquire() returns the
/// next available slot; Reset() recycles all slots for the next frame.
/// Grows in chunks if initial capacity is exceeded, but never relocates
/// existing elements (pointer stability guarantee).
template<typename T>
class HeapPool {
public:
    /// @brief Construct a pool with the given initial chunk capacity.
    /// @param chunkCapacity Number of elements per chunk.
    explicit HeapPool(size_t chunkCapacity = 256)
        : chunkCapacity_(chunkCapacity) {
        AddChunk();
    }

    ~HeapPool() {
        for (auto& chunk : chunks_) {
            for (size_t i = 0; i < chunk.constructed; ++i) {
                chunk.Ptr(i)->~T();
            }
            ::operator delete(chunk.storage);
        }
    }

    HeapPool(const HeapPool&) = delete;
    HeapPool& operator=(const HeapPool&) = delete;

    /// @brief Acquire the next available slot, default-constructing if new.
    /// @return Reference to a reusable, pointer-stable element.
    T& Acquire() {
        if (writeIdx_ >= TotalConstructed()) {
            // Need a new slot
            if (CurrentChunkFull()) {
                AddChunk();
            }
            auto& chunk = chunks_.back();
            T* ptr = chunk.Ptr(chunk.constructed);
            new (ptr) T();
            ++chunk.constructed;
        }
        T* result = GetSlot(writeIdx_);
        ++writeIdx_;
        if (writeIdx_ > highWaterMark_) {
            highWaterMark_ = writeIdx_;
        }
        return *result;
    }

    /// @brief Reset the write cursor to zero. All slots become reusable.
    /// Does not destroy elements - they are recycled in place.
    void Reset() { writeIdx_ = 0; }

    /// @brief Number of slots acquired since last Reset().
    size_t Count() const { return writeIdx_; }

    /// @brief Total pre-constructed slots available before growth.
    size_t Capacity() const { return TotalConstructed(); }

    /// @brief Peak Count() observed since construction.
    size_t HighWaterMark() const { return highWaterMark_; }

    /// @brief Number of backing chunks allocated.
    size_t ChunkCount() const { return chunks_.size(); }

private:
    struct Chunk {
        void* storage = nullptr;
        size_t capacity = 0;
        size_t constructed = 0;

        T* Ptr(size_t i) {
            return static_cast<T*>(storage) + i;
        }
    };

    size_t chunkCapacity_;
    size_t writeIdx_ = 0;
    size_t highWaterMark_ = 0;
    std::vector<Chunk> chunks_;

    void AddChunk() {
        Chunk c;
        c.storage = ::operator new(sizeof(T) * chunkCapacity_);
        c.capacity = chunkCapacity_;
        c.constructed = 0;
        chunks_.push_back(c);
    }

    bool CurrentChunkFull() const {
        return chunks_.empty() ||
               chunks_.back().constructed >= chunks_.back().capacity;
    }

    size_t TotalConstructed() const {
        size_t total = 0;
        for (auto& c : chunks_) {
            total += c.constructed;
        }
        return total;
    }

    T* GetSlot(size_t globalIndex) {
        for (auto& chunk : chunks_) {
            if (globalIndex < chunk.constructed) {
                return chunk.Ptr(globalIndex);
            }
            globalIndex -= chunk.constructed;
        }
        return nullptr;
    }
};

} // namespace koilo
