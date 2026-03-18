// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file debug_overlay.hpp
 * @brief On-screen debug overlay for watched CVars and expressions.
 *
 * @date 01/18/2026
 * @author Coela
 */
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace koilo {

/// Manages a list of watched expressions rendered as an on-screen overlay.
/// Access via the global g_debugOverlay pointer (set by KoiloInstance).
class DebugOverlay {
public:
    struct WatchEntry {
        std::string name;
        std::function<std::string()> getter;
    };

    DebugOverlay();
    ~DebugOverlay();

    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    /// Add a watch with a custom getter.
    void Add(const std::string& name, std::function<std::string()> getter);

    /// Remove a watch by name.
    bool Remove(const std::string& name);

    /// Remove all watches.
    void Clear();

    /// Check if there are any active watches.
    bool HasWatches() const;

    /// Get the number of active watches.
    size_t Count() const;

    /// Build the multi-line overlay text (one line per watch: "name = value").
    std::string BuildText() const;

    /// Get a copy of the current entries (for listing).
    std::vector<WatchEntry> Entries() const;

private:
    mutable std::mutex mutex_;
    std::vector<WatchEntry> entries_;
};

/// Global debug overlay pointer. Set by KoiloInstance, cleared on shutdown.
extern DebugOverlay* g_debugOverlay;

} // namespace koilo
