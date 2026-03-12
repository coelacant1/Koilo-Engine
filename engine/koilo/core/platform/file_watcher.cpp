// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file file_watcher.cpp
 * @brief FileWatcher implementation - stat-based file change polling.
 * @date 03/09/2026
 * @author Coela
 */

#include "file_watcher.hpp"
#include <sys/stat.h>
#include <algorithm>

namespace koilo {

int64_t FileWatcher::GetMtime(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) return 0;
    return static_cast<int64_t>(st.st_mtime);
}

void FileWatcher::Watch(const std::string& path) {
    // Avoid duplicates
    for (const auto& e : entries_) {
        if (e.path == path) return;
    }
    Entry entry;
    entry.path = path;
    entry.lastMtime = GetMtime(path);
    entries_.push_back(std::move(entry));
}

void FileWatcher::Unwatch(const std::string& path) {
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
                       [&](const Entry& e) { return e.path == path; }),
        entries_.end());
}

void FileWatcher::Clear() {
    entries_.clear();
}

int FileWatcher::Poll() {
    int changed = 0;
    for (auto& entry : entries_) {
        int64_t mtime = GetMtime(entry.path);
        if (mtime != entry.lastMtime && mtime != 0) {
            entry.lastMtime = mtime;
            ++changed;
            if (callback_) callback_(entry.path);
        }
    }
    return changed;
}

} // namespace koilo
