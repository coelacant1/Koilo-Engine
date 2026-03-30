// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file cancellation_token.hpp
 * @brief Cooperative cancellation token with parent-child linking.
 *
 * A CancellationToken is a shared atomic flag. Tasks check IsCancelled()
 * periodically and exit early when true. Parent tokens propagate
 * cancellation to all children.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

namespace koilo {

/// Cooperative cancellation token. Thread-safe, shareable.
class CancellationToken {
public:
    CancellationToken() = default;

    /// Create a child token linked to this parent.
    /// Cancelling the parent also cancels the child.
    std::shared_ptr<CancellationToken> CreateChild() {
        auto child = std::make_shared<CancellationToken>();
        std::lock_guard<std::mutex> lock(mutex_);
        children_.push_back(child);
        if (cancelled_.load(std::memory_order_relaxed)) {
            child->Cancel();
        }
        return child;
    }

    /// Request cancellation. Propagates to all children.
    void Cancel() {
        cancelled_.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& weak : children_) {
            if (auto child = weak.lock()) {
                child->Cancel();
            }
        }
    }

    /// Check if cancellation was requested.
    bool IsCancelled() const {
        return cancelled_.load(std::memory_order_acquire);
    }

    /// Reset to non-cancelled state (for reuse). Does NOT reset children.
    void Reset() {
        cancelled_.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> cancelled_{false};
    std::mutex mutex_;
    std::vector<std::weak_ptr<CancellationToken>> children_;
};

} // namespace koilo
