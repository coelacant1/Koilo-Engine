// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file particlepool.hpp
 * @brief Index-stable particle pool with free-list reuse.
 *
 * Soft-body math seed. Storage shell only - no solver here.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace koilo {

/**
 * @class ParticlePool
 * @brief Stable-handle storage for particle-like entries.
 */
template <typename T>
class ParticlePool {
public:
    using Handle = std::uint32_t;
    static constexpr Handle kInvalid = static_cast<Handle>(-1);

    ParticlePool() = default;

    Handle Add(const T& value) {
        if (!freeList_.empty()) {
            const Handle h = freeList_.back();
            freeList_.pop_back();
            data_[h] = value;
            alive_[h] = 1;
            ++liveCount_;
            return h;
        }
        const Handle h = static_cast<Handle>(data_.size());
        data_.push_back(value);
        alive_.push_back(1);
        ++liveCount_;
        return h;
    }

    void Remove(Handle h) {
        if (h >= data_.size() || !alive_[h]) return;
        alive_[h] = 0;
        freeList_.push_back(h);
        --liveCount_;
    }

    bool IsAlive(Handle h) const { return h < data_.size() && alive_[h] != 0; }

    T& Get(Handle h) { return data_[h]; }
    const T& Get(Handle h) const { return data_[h]; }

    std::size_t Capacity() const { return data_.size(); }
    std::size_t Size() const { return liveCount_; }

    void Clear() {
        data_.clear(); alive_.clear(); freeList_.clear(); liveCount_ = 0;
    }

    template <typename Fn>
    void ForEach(Fn&& fn) {
        for (std::size_t i = 0; i < data_.size(); ++i)
            if (alive_[i]) fn(static_cast<Handle>(i), data_[i]);
    }

private:
    std::vector<T> data_;
    std::vector<std::uint8_t> alive_;
    std::vector<Handle> freeList_;
    std::size_t liveCount_ = 0;
};

} // namespace koilo
