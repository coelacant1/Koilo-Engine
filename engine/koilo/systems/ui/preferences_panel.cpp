// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file preferences_panel.cpp
 * @brief Preferences panel implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "preferences_panel.hpp"

#include <cstdio>

namespace koilo {
namespace ui {

// ============================================================================
// Build
// ============================================================================

// Build the preferences UI as a FloatingPanel.
int PreferencesPanel::Build(UIContext& ctx, SettingsStore& store) {
    ctx_ = &ctx;
    store_ = &store;

    // Create floating panel
    panelIdx_ = ctx.CreateWidget(WidgetTag::FloatingPanel, "prefs_panel");
    if (panelIdx_ < 0) return -1;
    Widget* panel = ctx.Pool().Get(panelIdx_);
    panel->panelTitleId = ctx.Strings().Intern("Preferences");
    panel->localW = 500.0f;
    panel->localH = 400.0f;
    panel->localX = 150.0f;
    panel->localY = 80.0f;
    panel->widthMode = SizeMode::Fixed;
    panel->heightMode = SizeMode::Fixed;
    panel->layout.direction = LayoutDirection::Row;
    panel->flags.visible = false; // hidden by default

    // Category sidebar
    sidebarIdx_ = ctx.CreateWidget(WidgetTag::ScrollView, "prefs_sidebar");
    ctx.SetParent(sidebarIdx_, panelIdx_);
    Widget* sidebar = ctx.Pool().Get(sidebarIdx_);
    sidebar->layout.direction = LayoutDirection::Column;
    sidebar->widthMode = SizeMode::Fixed; sidebar->localW = 140.0f;
    sidebar->heightMode = SizeMode::Percent; sidebar->localH = 100.0f;

    // Content area
    contentIdx_ = ctx.CreateWidget(WidgetTag::ScrollView, "prefs_content");
    ctx.SetParent(contentIdx_, panelIdx_);
    Widget* content = ctx.Pool().Get(contentIdx_);
    content->layout.direction = LayoutDirection::Column;
    content->widthMode = SizeMode::Percent; content->localW = 100.0f;
    content->heightMode = SizeMode::Percent; content->localH = 100.0f;
    content->padding = {4, 4, 8, 8};

    // Bottom button row
    buttonRowIdx_ = ctx.CreatePanel("prefs_btns");
    ctx.SetParent(buttonRowIdx_, panelIdx_);
    Widget* btnRow = ctx.Pool().Get(buttonRowIdx_);
    btnRow->layout.direction = LayoutDirection::Row;
    btnRow->layout.mainAlign = Alignment::End;
    btnRow->widthMode = SizeMode::Percent; btnRow->localW = 100.0f;
    btnRow->heightMode = SizeMode::Fixed;  btnRow->localH = 32.0f;
    btnRow->padding = {4, 4, 4, 4};

    int applyBtn = ctx.CreateButton("prefs_apply", "Apply");
    ctx.SetParent(applyBtn, buttonRowIdx_);
    ctx.Pool().Get(applyBtn)->onClickCpp = [this](Widget&) {
        if (store_) store_->MarkClean();
        Hide();
    };

    int resetBtn = ctx.CreateButton("prefs_reset", "Reset Defaults");
    ctx.SetParent(resetBtn, buttonRowIdx_);
    ctx.Pool().Get(resetBtn)->onClickCpp = [this](Widget&) {
        if (store_) {
            store_->ResetAllDefaults();
            RebuildContent();
        }
    };

    int cancelBtn = ctx.CreateButton("prefs_cancel", "Cancel");
    ctx.SetParent(cancelBtn, buttonRowIdx_);
    ctx.Pool().Get(cancelBtn)->onClickCpp = [this](Widget&) { Hide(); };

    PopulateCategories();
    if (!categories_.empty()) SelectCategory(categories_[0]);

    return panelIdx_;
}

// ============================================================================
// Category Management
// ============================================================================

// Populate category sidebar buttons from store.
void PreferencesPanel::PopulateCategories() {
    if (!ctx_ || !store_ || sidebarIdx_ < 0) return;
    UIContext& ctx = *ctx_;

    // Clear sidebar
    Widget* sidebar = ctx.Pool().Get(sidebarIdx_);
    if (!sidebar) return;
    for (int i = 0; i < sidebar->childCount; ++i)
        if (sidebar->children[i] >= 0) ctx.Pool().Free(sidebar->children[i]);
    sidebar->childCount = 0;

    categories_ = store_->Categories();
    for (size_t i = 0; i < categories_.size(); ++i) {
        char btnId[64];
        std::snprintf(btnId, sizeof(btnId), "prefs_cat_%zu", i);
        int btn = ctx.CreateButton(btnId, categories_[i].c_str());
        ctx.SetParent(btn, sidebarIdx_);
        Widget* bw = ctx.Pool().Get(btn);
        bw->widthMode = SizeMode::Percent; bw->localW = 100.0f;
        bw->heightMode = SizeMode::Fixed;  bw->localH = 26.0f;

        std::string cat = categories_[i];
        bw->onClickCpp = [this, cat](Widget&) { SelectCategory(cat); };
    }
}

// ============================================================================
// Content Generation
// ============================================================================

// Rebuild content area for the currently selected category.
void PreferencesPanel::RebuildContent() {
    if (!ctx_ || !store_ || contentIdx_ < 0) return;
    UIContext& ctx = *ctx_;

    // Clear content
    Widget* content = ctx.Pool().Get(contentIdx_);
    if (!content) return;
    for (int i = 0; i < content->childCount; ++i)
        if (content->children[i] >= 0) ctx.Pool().Free(content->children[i]);
    content->childCount = 0;

    auto entries = store_->EntriesInCategory(selectedCategory_);
    for (size_t i = 0; i < entries.size(); ++i) {
        const SettingEntry* e = entries[i];
        char rowId[80];
        std::snprintf(rowId, sizeof(rowId), "prefs_row_%zu", i);
        int row = ctx.CreatePanel(rowId);
        ctx.SetParent(row, contentIdx_);
        Widget* rw = ctx.Pool().Get(row);
        rw->layout.direction = LayoutDirection::Row;
        rw->layout.crossAlign = Alignment::Center;
        rw->widthMode = SizeMode::Percent; rw->localW = 100.0f;
        rw->heightMode = SizeMode::Fixed;  rw->localH = 28.0f;
        rw->padding = {2, 2, 0, 0};

        // Label
        char lblId[80];
        std::snprintf(lblId, sizeof(lblId), "prefs_lbl_%zu", i);
        int lbl = ctx.CreateLabel(lblId, e->label.c_str());
        ctx.SetParent(lbl, row);
        Widget* lw = ctx.Pool().Get(lbl);
        lw->widthMode = SizeMode::Fixed; lw->localW = 160.0f;

        // Value widget based on variant type
        std::string key = e->key;
        std::visit([&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool>) {
                char cbId[80];
                std::snprintf(cbId, sizeof(cbId), "prefs_cb_%zu", i);
                int cb = ctx.CreateCheckbox(cbId, val);
                ctx.SetParent(cb, row);
                ctx.Pool().Get(cb)->onChangeCpp = [this, key](Widget& w) {
                    if (store_) store_->Set<bool>(key, w.checked);
                };
            } else if constexpr (std::is_same_v<T, int>) {
                char spId[80];
                std::snprintf(spId, sizeof(spId), "prefs_sp_%zu", i);
                int sp = ctx.CreateWidget(WidgetTag::NumberSpinner, spId);
                ctx.SetParent(sp, row);
                Widget* sw = ctx.Pool().Get(sp);
                sw->sliderValue = static_cast<float>(val);
                sw->sliderMin = -10000;
                sw->sliderMax = 10000;
                sw->spinnerStep = 1.0f;
                sw->widthMode = SizeMode::Fixed; sw->localW = 100.0f;
                sw->onChangeCpp = [this, key](Widget& w) {
                    if (store_) store_->Set<int>(key, static_cast<int>(w.sliderValue));
                };
            } else if constexpr (std::is_same_v<T, float>) {
                char slId[80];
                std::snprintf(slId, sizeof(slId), "prefs_sl_%zu", i);
                int sl = ctx.CreateSlider(slId, 0.0f, 100.0f, val);
                ctx.SetParent(sl, row);
                Widget* sw = ctx.Pool().Get(sl);
                sw->widthMode = SizeMode::Fixed; sw->localW = 140.0f;
                sw->onChangeCpp = [this, key](Widget& w) {
                    if (store_) store_->Set<float>(key, w.sliderValue);
                };
            } else if constexpr (std::is_same_v<T, std::string>) {
                char tfId[80];
                std::snprintf(tfId, sizeof(tfId), "prefs_tf_%zu", i);
                int tf = ctx.CreateTextField(tfId, val.c_str());
                ctx.SetParent(tf, row);
                Widget* tw = ctx.Pool().Get(tf);
                tw->widthMode = SizeMode::Fixed; tw->localW = 180.0f;
                tw->heightMode = SizeMode::Fixed; tw->localH = 22.0f;
            } else if constexpr (std::is_same_v<T, Color4>) {
                char cfId[80];
                std::snprintf(cfId, sizeof(cfId), "prefs_cf_%zu", i);
                int cf = ctx.CreateWidget(WidgetTag::ColorField, cfId);
                ctx.SetParent(cf, row);
                Widget* cw = ctx.Pool().Get(cf);
                cw->colorValue = val;
                cw->widthMode = SizeMode::Fixed; cw->localW = 60.0f;
                cw->heightMode = SizeMode::Fixed; cw->localH = 22.0f;
                cw->onChangeCpp = [this, key](Widget& w) {
                    if (store_) store_->Set<Color4>(key, w.colorValue);
                };
            }
        }, e->value);
    }
}

} // namespace ui
} // namespace koilo
