// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file content_browser.hpp
 * @brief Content browser panel - directory tree + file list + breadcrumb nav.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <koilo/systems/ui/breadcrumb.hpp>
#include <koilo/systems/ui/drag_drop.hpp>
#include <koilo/systems/ui/fs_adapter.hpp>
#include <koilo/systems/ui/ui_context.hpp>

namespace koilo {
namespace ui {

/** @class ContentBrowser @brief Content browser: tree sidebar + file list with breadcrumb and search. */
class ContentBrowser {
public:
    using FileActionCallback = std::function<void(const FsEntry&)>;

    /// Build the content browser inside a parent widget.
    int Build(UIContext& ctx, int parentIdx, const char* id);

    /// Navigate to a directory path and refresh contents.
    void NavigateTo(const std::string& path);

    /// Refresh the file list from the current path.
    void RefreshFileList();

    /// Refresh the directory tree sidebar.
    void RefreshTree();

    /** @brief Set the callback invoked when a file entry is clicked. */
    void SetOnFileAction(FileActionCallback cb) { onFileAction_ = std::move(cb); }

    /** @brief Set the callback invoked on double-click of a file entry. */
    void SetOnDoubleClick(FileActionCallback cb) { onDoubleClick_ = std::move(cb); }

    /** @brief Get the current directory path. */
    const std::string& CurrentPath() const { return currentPath_; }

    /** @brief Get the cached file-system entries. */
    const std::vector<FsEntry>& Entries() const { return entries_; }

    /** @brief Get the root widget pool index. */
    int RootIndex() const { return rootIdx_; }

    /** @brief Get the file-list widget pool index. */
    int ListIndex() const { return listIdx_; }

    /** @brief Get the tree sidebar widget pool index. */
    int TreeIndex() const { return treeIdx_; }

private:
    UIContext* ctx_ = nullptr;   ///< Owning UI context.
    int rootIdx_ = -1;           ///< Pool index of the root container.
    int topBarIdx_ = -1;         ///< Pool index of the top bar panel.
    int searchIdx_ = -1;         ///< Pool index of the search text field.
    int splitIdx_ = -1;          ///< Pool index of the tree/list split panel.
    int treeIdx_ = -1;           ///< Pool index of the tree sidebar.
    int listIdx_ = -1;           ///< Pool index of the file list.

    std::string currentPath_;              ///< Current directory being displayed.
    std::vector<FsEntry> entries_;         ///< Cached directory listing.

    Breadcrumb breadcrumb_;                ///< Breadcrumb navigation bar.
    FileActionCallback onFileAction_;      ///< Callback for single-click file action.
    FileActionCallback onDoubleClick_;     ///< Callback for double-click file action.
};

} // namespace ui
} // namespace koilo
