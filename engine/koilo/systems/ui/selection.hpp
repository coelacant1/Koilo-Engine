// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file selection.hpp
 * @brief Multi-object selection manager for the editor.
 *
 * Tracks a set of selected objects by opaque pointer + ClassDesc.
 * Fires a callback whenever the selection changes.
 *
 * @date 03/09/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/registry/registry.hpp>
#include <vector>
#include <functional>
#include <algorithm>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/// Callback signature: invoked when the selection set changes.
using SelectionChangedCallback = std::function<void()>;

/** @struct SelectionEntry @brief A single selected item - pairs a live pointer with its type. */
struct SelectionEntry {
    void* instance = nullptr;          ///< Opaque pointer to the selected object.
    const ClassDesc* desc = nullptr;   ///< Type descriptor for the selected object.

    bool operator==(const SelectionEntry& o) const {
        return instance == o.instance;
    }

    KL_BEGIN_FIELDS(SelectionEntry)
        /* Pointer fields not reflectable. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(SelectionEntry)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SelectionEntry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SelectionEntry)

};

/**
 * @class SelectionManager
 * @brief Manages a set of selected objects for editor inspection.
 */
class SelectionManager {
public:
    SelectionManager() = default;

    /** @brief Select a single object (clears previous selection). */
    void Select(void* instance, const ClassDesc* desc);

    /** @brief Add an object to the current selection (multi-select). */
    void AddToSelection(void* instance, const ClassDesc* desc);

    /** @brief Remove an object from the selection. */
    void RemoveFromSelection(void* instance);

    /** @brief Toggle an object in the selection. */
    void ToggleSelection(void* instance, const ClassDesc* desc);

    /** @brief Clear the selection. */
    void ClearSelection();

    /** @brief True if the given object is in the selection. */
    bool IsSelected(void* instance) const;

    /** @brief Number of selected objects. */
    size_t Count() const { return entries_.size(); }

    /** @brief True if exactly one object is selected. */
    bool IsSingle() const { return entries_.size() == 1; }

    /** @brief True if no objects are selected. */
    bool IsEmpty() const { return entries_.empty(); }

    /** @brief Get the first (or only) selected entry. Returns nullptr fields if empty. */
    const SelectionEntry& Primary() const { return entries_.empty() ? kEmpty_ : entries_[0]; }

    /** @brief Get all selected entries. */
    const std::vector<SelectionEntry>& Entries() const { return entries_; }

    /** @brief Set callback invoked when selection changes. */
    void SetOnChanged(SelectionChangedCallback cb) { onChange_ = std::move(cb); }

private:
    void NotifyChanged() { if (onChange_) onChange_(); }

    std::vector<SelectionEntry> entries_;   ///< Currently selected items.
    SelectionChangedCallback onChange_;      ///< Change notification callback.
    static const SelectionEntry kEmpty_;    ///< Sentinel for empty selection.

    KL_BEGIN_FIELDS(SelectionManager)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(SelectionManager)
        KL_METHOD_AUTO(SelectionManager, IsSingle, "Is single"),
        KL_METHOD_AUTO(SelectionManager, IsEmpty, "Is empty"),
        KL_METHOD_AUTO(SelectionManager, Primary, "Primary")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SelectionManager)
        KL_CTOR0(SelectionManager)
    KL_END_DESCRIBE(SelectionManager)

};

} // namespace koilo
