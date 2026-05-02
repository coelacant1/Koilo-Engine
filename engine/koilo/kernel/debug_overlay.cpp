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
    // F4: avoid the heavyweight std::ostringstream + per-call allocs.
    // A thread-local scratch string is reused across frames; only a final
    // copy on return is unavoidable (callers take std::string by value).
    thread_local std::string scratch;
    scratch.clear();
    for (auto& e : entries_) {
        std::string val;
        try { val = e.getter(); } catch (...) { val = "<error>"; }
        scratch.append(e.name);
        scratch.append(" = ", 3);
        scratch.append(val);
        scratch.push_back('\n');
    }
    return scratch;
}

std::vector<DebugOverlay::WatchEntry> DebugOverlay::Entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

} // namespace koilo
