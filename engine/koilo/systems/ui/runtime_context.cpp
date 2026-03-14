// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file runtime_context.cpp
 * @brief RuntimeContext implementation - isolated UI execution context.
 * @date 03/09/2026
 * @author Coela Can't
 */

#include "runtime_context.hpp"

namespace koilo {
namespace ui {

// ====================================================================
// Construction
// ====================================================================

// Initialise role, tag, and owned UIContext.
RuntimeContext::RuntimeContext(ContextRole role, size_t budget,
                               size_t poolCapacity)
    : role_(role)
    , tag_(role == ContextRole::Editor ? "Editor" : "Game")
    , ctx_(poolCapacity)
    , memoryBudget_(budget) {}

// ====================================================================
// Memory budget
// ====================================================================

// Attempt to reserve bytes; fail if over budget.
bool RuntimeContext::TryAllocate(size_t size) {
    if (memoryBudget_ > 0 && memoryUsage_ + size > memoryBudget_)
        return false;
    memoryUsage_ += size;
    if (memoryUsage_ > peakMemoryUsage_)
        peakMemoryUsage_ = memoryUsage_;
    return true;
}

// Decrease tracked usage, clamping to zero.
void RuntimeContext::Release(size_t size) {
    if (size > memoryUsage_)
        memoryUsage_ = 0;
    else
        memoryUsage_ -= size;
}

// ====================================================================
// Crash isolation
// ====================================================================

// Record an error message for display.
void RuntimeContext::SetError(const char* message) {
    hasError_ = true;
    lastError_ = message ? message : "Unknown error";
}

// Reset the error flag and message.
void RuntimeContext::ClearError() {
    hasError_ = false;
    lastError_.clear();
}

// ====================================================================
// Widget state snapshot / restore (hot-reload)
// ====================================================================

// Walk the pool and capture user-editable state for every named widget.
void RuntimeContext::SnapshotWidgetState() {
    snapshot_.clear();
    const WidgetPool& pool = ctx_.Pool();
    for (size_t i = 0; i < pool.Capacity(); ++i) {
        const Widget* w = pool.Get(static_cast<int>(i));
        if (!w) continue;
        if (w->id == NullStringId) continue;

        WidgetSnapshot snap;
        snap.id = w->id;
        snap.tag = w->tag;
        snap.sliderValue = w->sliderValue;
        snap.checked = w->checked;
        snap.expanded = w->expanded;
        snap.scrollX = w->scrollX;
        snap.scrollY = w->scrollY;
        snap.selectedIndex = w->selectedIndex;
        snapshot_.push_back(snap);
    }
}

// Match snapshot entries to new widgets by interned ID and restore values.
void RuntimeContext::RestoreWidgetState() {
    if (snapshot_.empty()) return;

    WidgetPool& pool = ctx_.Pool();
    for (const auto& snap : snapshot_) {
        // Linear scan to find widget with matching interned ID
        for (size_t i = 0; i < pool.Capacity(); ++i) {
            Widget* w = pool.Get(static_cast<int>(i));
            if (!w) continue;
            if (w->id != snap.id) continue;
            if (w->tag != snap.tag) continue;

            // Restore user-editable state
            w->sliderValue = snap.sliderValue;
            w->checked = snap.checked;
            w->expanded = snap.expanded;
            w->scrollX = snap.scrollX;
            w->scrollY = snap.scrollY;
            w->selectedIndex = snap.selectedIndex;
            break;
        }
    }
    snapshot_.clear();
}

} // namespace ui
} // namespace koilo
