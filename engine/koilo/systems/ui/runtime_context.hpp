// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file runtime_context.hpp
 * @brief Isolated execution context for editor or game scripts.
 *
 * A RuntimeContext bundles a UIContext, memory budget tracking, and
 * crash isolation.  Separate instances for the editor and the game
 * guarantee that a crash in user code never takes down the editor.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include "ui_context.hpp"
#include <koilo/registry/reflect_macros.hpp>
#include <string>
#include <cstddef>

namespace koilo {
namespace ui {

/// Identifies the role of a RuntimeContext.
enum class ContextRole : uint8_t {
    Editor = 0,
    Game
};

/**
 * @class RuntimeContext
 * @brief Owns an isolated UIContext plus crash/memory isolation state.
 *
 * Each RuntimeContext has:
 *  - Its own UIContext (widget tree, theme, events, layout)
 *  - A memory budget and current usage counter
 *  - Crash isolation flag (last error message)
 *  - An identity tag used for MemoryProfiler tracking
 */
class RuntimeContext {
public:
    /// @param role   Editor or Game
    /// @param budget Maximum memory budget in bytes (0 = unlimited)
    /// @param poolCapacity  Widget pool size for the UIContext
    explicit RuntimeContext(ContextRole role,
                            size_t budget = 0,
                            size_t poolCapacity = WidgetPool::DEFAULT_CAPACITY);

    ~RuntimeContext() = default;

    // Non-copyable, movable
    RuntimeContext(const RuntimeContext&) = delete;
    RuntimeContext& operator=(const RuntimeContext&) = delete;
    RuntimeContext(RuntimeContext&&) = default;
    RuntimeContext& operator=(RuntimeContext&&) = default;

    // -- Identity ------------------------------------------------
    ContextRole Role() const { return role_; }
    const char* Tag() const { return tag_; }

    // -- UIContext access ----------------------------------------
    UIContext& UI() { return ctx_; }
    const UIContext& UI() const { return ctx_; }

    // -- Memory budget -------------------------------------------

    /// Current tracked memory usage (bytes).
    size_t MemoryUsage() const { return memoryUsage_; }

    /// Maximum budget (bytes). 0 = unlimited.
    size_t MemoryBudget() const { return memoryBudget_; }

    /// Try to allocate size bytes within the budget.
    /// Returns true if within budget, false if would exceed it.
    bool TryAllocate(size_t size);

    /// Release tracked bytes.
    void Release(size_t size);

    /// Peak memory usage observed.
    size_t PeakMemoryUsage() const { return peakMemoryUsage_; }

    // -- Crash isolation -----------------------------------------

    /// True if the last script execution produced an error.
    bool HasError() const { return hasError_; }

    /// Human-readable error message from last failure.
    const char* LastError() const { return lastError_.c_str(); }

    /// Record an error (called by the engine when a script throws).
    void SetError(const char* message);

    /// Clear the error flag (e.g. after user acknowledges or on reload).
    void ClearError();

    // -- Widget tree snapshot for hot-reload ----------------------

    /// Capture serialisable state of every live widget (ID + values).
    /// Stores into an internal buffer for RestoreWidgetState().
    void SnapshotWidgetState();

    /// After a reload, walk the new widget tree and restore values
    /// for widgets whose interned IDs match the snapshot.
    void RestoreWidgetState();

private:
    ContextRole role_;
    const char* tag_;            ///< "Editor" or "Game" - used as memory profiler tag
    UIContext ctx_;

    size_t memoryBudget_  = 0;
    size_t memoryUsage_   = 0;
    size_t peakMemoryUsage_ = 0;

    bool hasError_ = false;
    std::string lastError_;

    // Hot-reload snapshot storage
    struct WidgetSnapshot {
        StringId id = NullStringId;
        WidgetTag tag = WidgetTag::Panel;
        float sliderValue = 0;
        bool checked = false;
        bool expanded = false;
        float scrollX = 0, scrollY = 0;
        int selectedIndex = 0;

        KL_BEGIN_FIELDS(WidgetSnapshot)
            KL_FIELD(WidgetSnapshot, sliderValue, "Slider value", 0, 0),
            KL_FIELD(WidgetSnapshot, checked, "Checked", 0, 1),
            KL_FIELD(WidgetSnapshot, expanded, "Expanded", 0, 1),
            KL_FIELD(WidgetSnapshot, scrollX, "Scroll X", 0, 0),
            KL_FIELD(WidgetSnapshot, scrollY, "Scroll Y", 0, 0),
            KL_FIELD(WidgetSnapshot, selectedIndex, "Selected index", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(WidgetSnapshot)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(WidgetSnapshot)
            /* No reflected ctors. */
        KL_END_DESCRIBE(WidgetSnapshot)

    };
    std::vector<WidgetSnapshot> snapshot_;

    KL_BEGIN_FIELDS(RuntimeContext)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(RuntimeContext)
        KL_METHOD_AUTO(RuntimeContext, Tag, "Tag"),
        KL_METHOD_AUTO(RuntimeContext, MemoryUsage, "Memory usage"),
        KL_METHOD_AUTO(RuntimeContext, MemoryBudget, "Memory budget"),
        KL_METHOD_AUTO(RuntimeContext, TryAllocate, "Try allocate"),
        KL_METHOD_AUTO(RuntimeContext, HasError, "Has error"),
        KL_METHOD_AUTO(RuntimeContext, LastError, "Last error")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RuntimeContext)
        /* No default ctor - requires role + tag. */
    KL_END_DESCRIBE(RuntimeContext)

};

} // namespace ui
} // namespace koilo
