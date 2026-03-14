// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file selection.cpp
 * @brief SelectionManager implementation.
 * @date 03/09/2026
 * @author Coela Can't
 */

#include "selection.hpp"

namespace koilo {

// =====================================================================
// Static members
// =====================================================================

const SelectionEntry SelectionManager::kEmpty_ = {};

// =====================================================================
// Selection operations
// =====================================================================

// Replace the selection with a single object
void SelectionManager::Select(void* instance, const ClassDesc* desc) {
    entries_.clear();
    if (instance) {
        entries_.push_back({instance, desc});
    }
    NotifyChanged();
}

// Append an object without clearing existing selection
void SelectionManager::AddToSelection(void* instance, const ClassDesc* desc) {
    if (!instance) return;
    if (IsSelected(instance)) return;
    entries_.push_back({instance, desc});
    NotifyChanged();
}

// Remove a specific object from the selection
void SelectionManager::RemoveFromSelection(void* instance) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [instance](const SelectionEntry& e) { return e.instance == instance; });
    if (it != entries_.end()) {
        entries_.erase(it);
        NotifyChanged();
    }
}

// Add if absent, remove if present
void SelectionManager::ToggleSelection(void* instance, const ClassDesc* desc) {
    if (IsSelected(instance)) {
        RemoveFromSelection(instance);
    } else {
        AddToSelection(instance, desc);
    }
}

// Drop all selected objects
void SelectionManager::ClearSelection() {
    if (!entries_.empty()) {
        entries_.clear();
        NotifyChanged();
    }
}

// Check whether an object is currently selected
bool SelectionManager::IsSelected(void* instance) const {
    return std::any_of(entries_.begin(), entries_.end(),
        [instance](const SelectionEntry& e) { return e.instance == instance; });
}

} // namespace koilo
