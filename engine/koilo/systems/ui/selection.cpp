// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file selection.cpp
 * @brief SelectionManager implementation.
 * @date 03/09/2026
 * @author Coela
 */

#include "selection.hpp"

namespace koilo {

const SelectionEntry SelectionManager::kEmpty_ = {};

void SelectionManager::Select(void* instance, const ClassDesc* desc) {
    entries_.clear();
    if (instance) {
        entries_.push_back({instance, desc});
    }
    NotifyChanged();
}

void SelectionManager::AddToSelection(void* instance, const ClassDesc* desc) {
    if (!instance) return;
    if (IsSelected(instance)) return;
    entries_.push_back({instance, desc});
    NotifyChanged();
}

void SelectionManager::RemoveFromSelection(void* instance) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [instance](const SelectionEntry& e) { return e.instance == instance; });
    if (it != entries_.end()) {
        entries_.erase(it);
        NotifyChanged();
    }
}

void SelectionManager::ToggleSelection(void* instance, const ClassDesc* desc) {
    if (IsSelected(instance)) {
        RemoveFromSelection(instance);
    } else {
        AddToSelection(instance, desc);
    }
}

void SelectionManager::ClearSelection() {
    if (!entries_.empty()) {
        entries_.clear();
        NotifyChanged();
    }
}

bool SelectionManager::IsSelected(void* instance) const {
    return std::any_of(entries_.begin(), entries_.end(),
        [instance](const SelectionEntry& e) { return e.instance == instance; });
}

} // namespace koilo
