// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file file_watcher.hpp
 * @brief Lightweight file modification watcher for hot-reload support.
 *
 * Polls stat() timestamps on watched paths. Call Poll() each frame to
 * detect changes.  No background threads - designed for frame-budget use.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/// Callback signature: (path that changed).
using FileChangedCallback = std::function<void(const std::string& path)>;

/**
 * @class FileWatcher
 * @brief Polls a set of file paths for modification time changes.
 */
class FileWatcher {
public:
    FileWatcher() = default;
    ~FileWatcher() = default;

    /// Add a file path to watch.  Records its current mtime.
    void Watch(const std::string& path);

    /// Remove a path from the watch list.
    void Unwatch(const std::string& path);

    /// Remove all watched paths.
    void Clear();

    /// Set the callback invoked when a file changes.
    void SetCallback(FileChangedCallback cb) { callback_ = std::move(cb); }

    /// Poll all watched files.  Invokes the callback for any that changed.
    /// Returns the number of files that changed.
    int Poll();

    /// Number of watched files.
    size_t Count() const { return entries_.size(); }

private:
    struct Entry {
        std::string path;
        int64_t lastMtime = 0;

        KL_BEGIN_FIELDS(Entry)
            KL_FIELD(Entry, path, "Path", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Entry)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Entry)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Entry)

    };

    static int64_t GetMtime(const std::string& path);

    std::vector<Entry> entries_;
    FileChangedCallback callback_;

    KL_BEGIN_FIELDS(FileWatcher)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(FileWatcher)
        KL_METHOD_AUTO(FileWatcher, Poll, "Poll")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FileWatcher)
        KL_CTOR0(FileWatcher)
    KL_END_DESCRIBE(FileWatcher)

};

} // namespace koilo
