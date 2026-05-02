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

// =====================================================================
// Multi-listener subscription
// =====================================================================

SelectionListenerToken SelectionManager::Subscribe(SelectionChangedCallback cb) {
    if (!cb) return 0;
    SelectionListenerToken token = nextToken_++;
    listeners_.push_back({token, std::move(cb)});
    return token;
}

void SelectionManager::Unsubscribe(SelectionListenerToken token) {
    if (token == 0) return;
    // Erase-remove; safe even if invoked from inside NotifyChanged()
    // because NotifyChanged copies the list before iterating.
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
            [token](const Listener& l) { return l.token == token; }),
        listeners_.end());
}

void SelectionManager::NotifyChanged() {
    // Re-entrancy guard: a callback that mutates the selection
    // shouldn't recursively re-fire all listeners - the outer call
    // will still complete the original notification round.
    if (notifying_) return;
    notifying_ = true;

    // Snapshot listeners so Unsubscribe()/Subscribe() inside a
    // callback doesn't invalidate our iterator.
    auto snapshot = listeners_;
    for (const auto& l : snapshot) {
        if (l.cb) l.cb();
    }
    if (onChange_) onChange_();

    notifying_ = false;
}

} // namespace koilo
