// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file breadcrumb.hpp
 * @brief Breadcrumb navigation widget - clickable path segments with separators.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <koilo/systems/ui/ui_context.hpp>

namespace koilo {
namespace ui {

/** @class Breadcrumb
 *  @brief Breadcrumb navigation bar built from path segments.
 *
 *  Each segment is a clickable button, separated by "›" labels.
 */
class Breadcrumb {
public:
    using NavigateCallback = std::function<void(const std::string& path)>; ///< Callback type for segment clicks.

    /** @brief Build the breadcrumb bar inside a parent widget.
     *  @param ctx        UI context used to create widgets.
     *  @param parentIdx  Parent widget index.
     *  @param id         Widget identifier string.
     *  @return Row container widget index, or -1 on failure.
     */
    int Build(UIContext& ctx, int parentIdx, const char* id);

    /** @brief Set the current path and rebuild segments.
     *  @param path  New path string.
     */
    void SetPath(const std::string& path) {
        if (!ctx_ || rowIdx_ < 0) return;
        currentPath_ = path;
        RebuildSegments();
    }

    /** @brief Set callback for when a segment is clicked.
     *  @param cb  Navigation callback.
     */
    void SetOnNavigate(NavigateCallback cb) { onNavigate_ = std::move(cb); }

    /** @brief Get the current path string. */
    const std::string& CurrentPath() const { return currentPath_; }

    /** @brief Get the row container widget index. */
    int RowIndex() const { return rowIdx_; }

private:
    void RebuildSegments();

    UIContext*   ctx_ = nullptr;  ///< Borrowed UI context.
    int          rowIdx_ = -1;   ///< Row container widget index.
    std::string  currentPath_;   ///< Full path string.
    std::vector<std::string> segments_; ///< Parsed path segments.
    NavigateCallback onNavigate_; ///< Navigation callback.
};

} // namespace ui
} // namespace koilo
