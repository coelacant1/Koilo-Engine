// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file preferences_panel.hpp
 * @brief Preferences dialog - category sidebar + auto-generated settings UI.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <koilo/systems/ui/settings_store.hpp>
#include <koilo/systems/ui/ui_context.hpp>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/** @class PreferencesPanel
 *  @brief Preferences dialog panel.
 *
 *  Reads SettingsStore categories/entries and emits widget rows for each setting.
 */
class PreferencesPanel {
public:
    /** @brief Build the preferences UI as a FloatingPanel.
     *  @param ctx  UI context used to create widgets.
     *  @param store  Settings store providing categories and entries.
     *  @return Widget index of the created panel, or -1 on failure.
     */
    int Build(UIContext& ctx, SettingsStore& store);

    /** @brief Show the preferences panel. */
    void Show() {
        if (ctx_ && panelIdx_ >= 0) {
            Widget* p = ctx_->Pool().Get(panelIdx_);
            if (p) p->flags.visible = true;
        }
    }

    /** @brief Hide the preferences panel. */
    void Hide() {
        if (ctx_ && panelIdx_ >= 0) {
            Widget* p = ctx_->Pool().Get(panelIdx_);
            if (p) p->flags.visible = false;
        }
    }

    /** @brief Check whether the preferences panel is currently visible. */
    bool IsVisible() const {
        if (ctx_ && panelIdx_ >= 0) {
            const Widget* p = ctx_->Pool().Get(panelIdx_);
            return p && p->flags.visible;
        }
        return false;
    }

    /** @brief Select a category and rebuild the content area.
     *  @param cat  Category name to select.
     */
    void SelectCategory(const std::string& cat) {
        selectedCategory_ = cat;
        RebuildContent();
    }

    /** @brief Get the widget index of the panel. */
    int PanelIndex() const { return panelIdx_; }

    /** @brief Get the currently selected category name. */
    const std::string& SelectedCategory() const { return selectedCategory_; }

private:
    void PopulateCategories();

    void RebuildContent();

    UIContext*     ctx_   = nullptr; ///< Borrowed UI context.
    SettingsStore* store_ = nullptr; ///< Borrowed settings store.
    int panelIdx_     = -1; ///< Root floating-panel widget index.
    int sidebarIdx_   = -1; ///< Category sidebar scroll-view index.
    int contentIdx_   = -1; ///< Content area scroll-view index.
    int buttonRowIdx_ = -1; ///< Bottom button row widget index.
    std::string selectedCategory_; ///< Currently selected category name.
    std::vector<std::string> categories_; ///< Cached list of category names.

    KL_BEGIN_FIELDS(PreferencesPanel)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(PreferencesPanel)
        /* Unsupported overload: PreferencesPanel::if */
        KL_METHOD_AUTO(PreferencesPanel, Hide, "Hide"),
        /* Unsupported overload: PreferencesPanel::if */
        KL_METHOD_AUTO(PreferencesPanel, IsVisible, "Is visible"),
        KL_METHOD_AUTO(PreferencesPanel, SelectCategory, "Select category"),
        KL_METHOD_AUTO(PreferencesPanel, RebuildContent, "Rebuild content"),
        KL_METHOD_AUTO(PreferencesPanel, PanelIndex, "Panel index"),
        KL_METHOD_AUTO(PreferencesPanel, SelectedCategory, "Selected category")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PreferencesPanel)
        /* No reflected ctors. */
    KL_END_DESCRIBE(PreferencesPanel)

};

} // namespace ui
} // namespace koilo
