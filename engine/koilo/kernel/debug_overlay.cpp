// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/debug_overlay.hpp>
#include <algorithm>
#include <sstream>

namespace koilo {

DebugOverlay* g_debugOverlay = nullptr;

DebugOverlay::DebugOverlay() {
    g_debugOverlay = this;
}

DebugOverlay::~DebugOverlay() {
    if (g_debugOverlay == this) g_debugOverlay = nullptr;
}

void DebugOverlay::Add(const std::string& name, std::function<std::string()> getter) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Replace if exists
    for (auto& e : entries_) {
        if (e.name == name) { e.getter = std::move(getter); return; }
    }
    entries_.push_back({name, std::move(getter)});
}

bool DebugOverlay::Remove(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(entries_.begin(), entries_.end(),
        [&](const WatchEntry& e) { return e.name == name; });
    if (it == entries_.end()) return false;
    entries_.erase(it, entries_.end());
    return true;
}

void DebugOverlay::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

bool DebugOverlay::HasWatches() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !entries_.empty();
}

size_t DebugOverlay::Count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

std::string DebugOverlay::BuildText() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream os;
    for (auto& e : entries_) {
        std::string val;
        try { val = e.getter(); } catch (...) { val = "<error>"; }
        os << e.name << " = " << val << "\n";
    }
    return os.str();
}

std::vector<DebugOverlay::WatchEntry> DebugOverlay::Entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

} // namespace koilo
