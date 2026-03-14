// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui.cpp
 * @brief UI class implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/ui/auto_inspector.hpp>
#include <koilo/systems/input/keycodes.hpp>
#include <koilo/registry/global_registry.hpp>
#include <koilo/scripting/reflection_bridge.hpp>
#include <cstring>
#include <cmath>

namespace koilo {

// ============================================================================
// Constructor
// ============================================================================

UI::UI() {
    int root = ctx_.CreatePanel("__root");
    ctx_.SetRoot(root);
    ctx_.SetSizeMode(root, ui::SizeMode::FillRemaining,
                           ui::SizeMode::FillRemaining);
}

// ============================================================================
// Viewport & Layout
// ============================================================================

void UI::SetViewport(float w, float h) { ctx_.SetViewport(w, h); }
void UI::UpdateLayout() {
    // Compute delta time for transitions
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;
    if (dt > 0.1f) dt = 0.1f; // clamp to avoid jumps

    ctx_.UpdateTransitions(dt);
    AutoSizeTextWidgets();
    ctx_.UpdateLayout();
}

// ============================================================================
// Widget Creation
// ============================================================================

/// Create a panel widget and attach to root.
int UI::CreatePanel(const char* id) {
    int idx = ctx_.CreatePanel(id);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a label widget and attach to root.
int UI::CreateLabel(const char* id, const char* text) {
    int idx = ctx_.CreateLabel(id, text);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a button widget and attach to root.
int UI::CreateButton(const char* id, const char* text) {
    int idx = ctx_.CreateButton(id, text);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a slider widget and attach to root.
int UI::CreateSlider(const char* id, float min, float max, float value) {
    int idx = ctx_.CreateSlider(id, min, max, value);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a checkbox widget and attach to root.
int UI::CreateCheckbox(const char* id, bool checked) {
    int idx = ctx_.CreateCheckbox(id, checked);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a text field widget and attach to root.
int UI::CreateTextField(const char* id, const char* placeholder) {
    int idx = ctx_.CreateTextField(id, placeholder);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a separator widget and attach to root.
int UI::CreateSeparator(const char* id) {
    int idx = ctx_.CreateSeparator(id);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a scroll view widget with clip and fill behavior.
int UI::CreateScrollView(const char* id) {
    int idx = ctx_.CreateWidget(ui::WidgetTag::ScrollView, id);
    if (idx >= 0) {
        ctx_.SetParent(idx, ctx_.Root());
        ctx_.SetSizeMode(idx, ui::SizeMode::FillRemaining, ui::SizeMode::FillRemaining);
        ui::Widget* w = ctx_.GetWidget(idx);
        if (w) w->flags.clipChildren = 1;
    }
    return idx;
}

/// Create a tree node widget and attach to root.
int UI::CreateTreeNode(const char* id, const char* text) {
    int idx = ctx_.CreateWidget(ui::WidgetTag::TreeNode, id);
    if (idx >= 0) {
        ctx_.SetParent(idx, ctx_.Root());
        ctx_.SetText(idx, text);
        ctx_.SetSizeMode(idx, ui::SizeMode::FillRemaining, ui::SizeMode::FitContent);
    }
    return idx;
}

/// Create a dock container widget and attach to root.
int UI::CreateDockContainer(const char* id) {
    int idx = ctx_.CreateDockContainer(id);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a split pane widget and attach to root.
int UI::CreateSplitPane(const char* id, bool vertical) {
    int idx = ctx_.CreateSplitPane(id, vertical);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a tab bar widget and attach to root.
int UI::CreateTabBar(const char* id) {
    int idx = ctx_.CreateTabBar(id);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a popup menu widget and attach to root.
int UI::CreatePopupMenu(const char* id) {
    int idx = ctx_.CreatePopupMenu(id);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

/// Create a menu item widget and attach to root.
int UI::CreateMenuItem(const char* id, const char* text) {
    int idx = ctx_.CreateMenuItem(id, text);
    if (idx >= 0) ctx_.SetParent(idx, ctx_.Root());
    return idx;
}

// ============================================================================
// Widget Tree
// ============================================================================

/// Set widget parent-child relationship.
bool UI::SetParent(int child, int parent) { return ctx_.SetParent(child, parent); }
/// Destroy a widget by index.
void UI::DestroyWidget(int idx) { ctx_.DestroyWidget(idx); }
/// Find a widget by string ID.
int UI::FindWidget(const char* id) { return ctx_.FindWidget(id); }
/// Get the root widget index.
int UI::GetRoot() { return ctx_.Root(); }
/// Get the number of live widgets.
int UI::GetWidgetCount() { return static_cast<int>(ctx_.Pool().Count()); }

// ============================================================================
// Properties
// ============================================================================

/// Set the text of a widget.
void UI::SetText(int idx, const char* text) { ctx_.SetText(idx, text); }
/// Get the text of a widget.
const char* UI::GetText(int idx) { return ctx_.GetText(idx); }
/// Set widget visibility.
void UI::SetVisible(int idx, bool v) { ctx_.SetVisible(idx, v); }
/// Set widget enabled state.
void UI::SetEnabled(int idx, bool e) { ctx_.SetEnabled(idx, e); }
/// Set widget selected state.
void UI::SetSelected(int idx, bool s) { ctx_.SetSelected(idx, s); }

/// Set a ColorField's color from a hex string.
void UI::SetColorHex(int idx, const char* hex) {
    ui::Widget* w = ctx_.Pool().Get(idx);
    if (!w || !hex) return;
    w->colorValue = ui::Color4::FromHex(hex);
    ctx_.SetRenderDirty();
}

/// Set widget size.
void UI::SetSize(int idx, float w, float h) { ctx_.SetSize(idx, w, h); }
/// Set widget position.
void UI::SetPosition(int idx, float x, float y) { ctx_.SetPosition(idx, x, y); }

/// Set widget padding (top, right, bottom, left).
void UI::SetPadding(int idx, float top, float right, float bottom, float left) {
    ctx_.SetPadding(idx, top, right, bottom, left);
}

/// Set widget margin (top, right, bottom, left).
void UI::SetMargin(int idx, float top, float right, float bottom, float left) {
    ctx_.SetMargin(idx, top, right, bottom, left);
}

// ============================================================================
// Layout
// ============================================================================

/// Set column layout with spacing.
void UI::SetLayoutColumn(int idx, float spacing) {
    ctx_.SetLayout(idx, ui::LayoutDirection::Column,
                   ui::Alignment::Start, ui::Alignment::Start, spacing);
}

/// Set row layout with spacing.
void UI::SetLayoutRow(int idx, float spacing) {
    ctx_.SetLayout(idx, ui::LayoutDirection::Row,
                   ui::Alignment::Start, ui::Alignment::Start, spacing);
}

/// Set widget to fill available width.
void UI::SetFillWidth(int idx) {
    ctx_.SetSizeMode(idx, ui::SizeMode::FillRemaining,
                     ctx_.GetWidget(idx) ? ctx_.GetWidget(idx)->heightMode
                                         : ui::SizeMode::Fixed);
}

/// Set widget to fill available height.
void UI::SetFillHeight(int idx) {
    ui::SizeMode wMode = ctx_.GetWidget(idx) ? ctx_.GetWidget(idx)->widthMode
                                              : ui::SizeMode::Fixed;
    ctx_.SetSizeMode(idx, wMode, ui::SizeMode::FillRemaining);
}

// ============================================================================
// Events
// ============================================================================

/// Set click callback script function name.
void UI::SetOnClick(int idx, const char* fnName) { ctx_.SetOnClickScript(idx, fnName); }
/// Set change callback script function name.
void UI::SetOnChange(int idx, const char* fnName) { ctx_.SetOnChangeScript(idx, fnName); }

// ============================================================================
// Query
// ============================================================================

/// Get the current slider value.
float UI::GetSliderValue(int idx) {
    const ui::Widget* w = ctx_.GetWidget(idx);
    return w ? w->sliderValue : 0.0f;
}

/// Get checkbox checked state.
bool UI::GetChecked(int idx) {
    const ui::Widget* w = ctx_.GetWidget(idx);
    return w ? w->checked : false;
}

/// Get viewport width.
float UI::GetViewportWidth() { return ctx_.ViewportWidth(); }
/// Get viewport height.
float UI::GetViewportHeight() { return ctx_.ViewportHeight(); }

// ============================================================================
// Inspector
// ============================================================================

/// Auto-generate inspector UI for a registered type.
int UI::InspectType(const char* className, int parentIdx) {
    if (!className) return -1;
    const ClassDesc* desc = scripting::ReflectionBridge::FindClass(className);
    if (!desc) return -1;
    void* instance = scripting::ReflectionBridge::CreateInstance(desc);
    if (!instance) return -1;
    ui::InspectorResult r = ui::GenerateInspector(desc, instance, ctx_, parentIdx);
    return r.rootWidget;
}

// ============================================================================
// Theme
// ============================================================================

/// Override theme color for a widget element type.
void UI::SetThemeColor(const char* element, int r, int g, int b, int a) {
    if (!element) return;
    ui::Color4 c(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                 static_cast<uint8_t>(b), static_cast<uint8_t>(a));

    // Map element name to WidgetTag for background color override
    struct TagMap { const char* name; ui::WidgetTag tag; };
    static const TagMap tags[] = {
        {"panel",     ui::WidgetTag::Panel},
        {"label",     ui::WidgetTag::Label},
        {"button",    ui::WidgetTag::Button},
        {"textfield", ui::WidgetTag::TextField},
        {"slider",    ui::WidgetTag::Slider},
        {"checkbox",  ui::WidgetTag::Checkbox},
        {"separator", ui::WidgetTag::Separator},
        {"dock",      ui::WidgetTag::DockContainer},
        {"split",     ui::WidgetTag::SplitPane},
        {"tab",       ui::WidgetTag::TabBar},
        {"popup",     ui::WidgetTag::PopupMenu},
        {"menuitem",  ui::WidgetTag::MenuItem},
    };

    for (const auto& t : tags) {
        if (strcmp(element, t.name) == 0) {
            ui::Style s = ctx_.GetTheme().Get(t.tag, ui::PseudoState::Normal);
            s.background = c;
            s.isSet = true;
            ctx_.GetTheme().Set(t.tag, ui::PseudoState::Normal, s);
            return;
        }
    }
}

// ============================================================================
// Auto-size Text Widgets
// ============================================================================

/// Pre-layout pass: auto-size text widgets without explicit dimensions.
void UI::AutoSizeTextWidgets() {
    if (!font_.IsLoaded()) return;

    auto& pool = ctx_.Pool();
    auto& strings = ctx_.Strings();
    const auto& theme = ctx_.GetTheme();

    for (int i = 0; i < static_cast<int>(pool.Capacity()); ++i) {
        if (!pool.IsAlive(i)) continue;
        ui::Widget* w = pool.Get(i);
        if (!w || !w->flags.visible) continue;

        // Only auto-size widgets that have text and are using default 0 dimensions
        bool hasText = (w->tag == ui::WidgetTag::Label ||
                        w->tag == ui::WidgetTag::Button ||
                        w->tag == ui::WidgetTag::MenuItem ||
                        w->tag == ui::WidgetTag::TreeNode);
        if (!hasText) continue;

        const char* text = strings.Lookup(w->textId);
        if (!text || text[0] == '\0') continue;

        float scale = w->fontSize > 0.0f ? (w->fontSize / font_.PixelSize()) : 1.0f;
        auto metrics = font_.MeasureText(text);

        // Use effective padding (same logic as renderer: widget padding if any nonzero,
        // else theme/per-widget style padding)
        bool hasWidgetPad = (w->padding.top != 0.0f || w->padding.right != 0.0f ||
                             w->padding.bottom != 0.0f || w->padding.left != 0.0f);
        ui::Edges pad = hasWidgetPad ? w->padding
                                     : theme.Resolve(*w, i).padding;

        // Auto-size width if not explicitly set (Auto/FitContent or Fixed with default 0)
        if (w->widthMode == ui::SizeMode::Auto ||
            w->widthMode == ui::SizeMode::FitContent ||
            w->widthMode == ui::SizeMode::MinContent ||
            w->widthMode == ui::SizeMode::MaxContent ||
            (w->widthMode == ui::SizeMode::Fixed && w->localW <= 0.0f)) {
            // Account for letter-spacing (applied between characters)
            float lsExtra = 0.0f;
            if (w->letterSpacing != 0.0f) {
                size_t charCount = std::strlen(text);
                if (charCount > 1) lsExtra = w->letterSpacing * (charCount - 1);
            }
            // Ceil + 1px buffer to account for per-glyph rounding in renderer
            float textW = std::ceil(metrics.width * scale + lsExtra) + 1.0f;
            w->localW = textW + pad.left + pad.right;
        }

        // Auto-size height if not explicitly set
        if (w->heightMode == ui::SizeMode::Auto ||
            w->heightMode == ui::SizeMode::FitContent ||
            w->heightMode == ui::SizeMode::MinContent ||
            w->heightMode == ui::SizeMode::MaxContent ||
            (w->heightMode == ui::SizeMode::Fixed && w->localH <= 0.0f)) {
            w->localH = metrics.height * scale + pad.top + pad.bottom;
        }
    }
}

/// Render UI to a Color888 buffer via software path.
void UI::RenderToBuffer(Color888* buffer, int width, int height) {
    // Skip if the root has no children - the opaque clear would overwrite
    // the scene/canvas content with the UI background color.
    auto* root = ctx_.GetWidget(ctx_.Root());
    if (!root || root->childCount == 0) return;

    ctx_.SetViewport(static_cast<float>(width), static_cast<float>(height));
    AutoSizeTextWidgets();
    ctx_.UpdateLayout();

    if (!buffer) return;

    // Build draw commands from widget tree
    drawList_.Clear();
    font::Font* f = font_.IsLoaded() ? &font_ : nullptr;
    drawList_.BuildFromContext(ctx_, f, 0);

    // Render via software path
    swRenderer_.Resize(width, height);
    swRenderer_.Clear(ui::Color4{30, 30, 30, 255});
    swRenderer_.Render(drawList_, f ? &f->Atlas() : nullptr);

    // Composite UI pixels on top of existing buffer (alpha blend)
    const uint8_t* uiPixels = swRenderer_.Pixels();
    for (int i = 0; i < width * height; ++i) {
        int si = i * 4;
        uint8_t ua = uiPixels[si + 3];
        if (ua == 0) continue;
        if (ua == 255) {
            buffer[i].r = uiPixels[si + 0];
            buffer[i].g = uiPixels[si + 1];
            buffer[i].b = uiPixels[si + 2];
        } else {
            uint8_t inv = 255 - ua;
            buffer[i].r = static_cast<uint8_t>((uiPixels[si+0] * ua + buffer[i].r * inv) / 255);
            buffer[i].g = static_cast<uint8_t>((uiPixels[si+1] * ua + buffer[i].g * inv) / 255);
            buffer[i].b = static_cast<uint8_t>((uiPixels[si+2] * ua + buffer[i].b * inv) / 255);
        }
    }
}

/// Load a TTF font for UI text rendering.
bool UI::LoadFont(const char* path, float pixelSize) {
    return font_.LoadFromFile(path, pixelSize);
}

/// Check if a font is loaded.
bool UI::HasFont() const { return font_.IsLoaded(); }

/// Initialize GPU UI rendering (OpenGL).
bool UI::InitializeGPU() {
    if (gpuInitialized_) return true;
    if (!glRenderer_.Initialize()) return false;
    if (font_.IsLoaded()) {
        fontAtlasTexture_ = glRenderer_.UploadFontAtlas(font_.Atlas());
    }
    gpuInitialized_ = true;
    return true;
}

/// Render UI overlay via OpenGL.
void UI::RenderGPU(int viewportW, int viewportH) {
    ctx_.SetViewport(static_cast<float>(viewportW), static_cast<float>(viewportH));

    // Update theme transitions (hover/press/focus style interpolation)
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;
    if (dt > 0.1f) dt = 0.1f;
    ctx_.UpdateTransitions(dt);

    // Skip draw list rebuild when nothing changed; re-render cached list
    if (!ctx_.IsRenderDirty()) {
        if (drawList_.Size() > 0)
            glRenderer_.Render(drawList_, viewportW, viewportH);
        return;
    }

    AutoSizeTextWidgets();
    ctx_.UpdateLayout();

    // Upload font atlas if font loaded after GPU init
    if (font_.IsLoaded() && fontAtlasTexture_ == 0 && gpuInitialized_) {
        fontAtlasTexture_ = glRenderer_.UploadFontAtlas(font_.Atlas());
    }

    font::Font* f = font_.IsLoaded() ? &font_ : nullptr;

    // Build draw commands - may rasterize new glyphs into the atlas.
    // If the atlas grows mid-build, UV coords in already-emitted commands
    // become stale, so we detect growth and rebuild.
    int gen = f ? f->Atlas().Generation() : 0;
    drawList_.Clear();
    drawList_.BuildFromContext(ctx_, f, fontAtlasTexture_);
    if (f && f->Atlas().Generation() != gen) {
        // Atlas grew - all glyphs are now cached, rebuild with correct UVs
        drawList_.Clear();
        drawList_.BuildFromContext(ctx_, f, fontAtlasTexture_);
    }

    ctx_.ClearRenderDirty();

    // Re-upload atlas if new glyphs were rasterized during BuildFromContext
    if (font_.IsLoaded() && fontAtlasTexture_ != 0 && font_.Atlas().IsDirty()) {
        glRenderer_.UploadFontAtlas(font_.Atlas());
    }

    glRenderer_.Render(drawList_, viewportW, viewportH);
}

/// Remove all widgets and reset state.
void UI::Clear() {
    auto& pool = ctx_.Pool();
    for (size_t i = 0; i < pool.Capacity(); ++i) {
        if (pool.IsAlive(static_cast<int>(i))) {
            pool.Free(static_cast<int>(i));
        }
    }
    tweenPool_.CancelAll(-1); // cancel all by passing invalid widget (won't match any)
    // manually clear all tweens
    for (size_t i = 0; i < tweenPool_.Capacity(); ++i) {
        tweenPool_.Cancel(static_cast<int>(i));
    }
    int root = ctx_.CreatePanel("__root");
    ctx_.SetRoot(root);
    ctx_.SetSizeMode(root, ui::SizeMode::FillRemaining,
                           ui::SizeMode::FillRemaining);
}

// ============================================================================
// Markup Loading (KML + KSS)
// ============================================================================

/// Load a UI layout from KML markup and optional KSS stylesheet.
int UI::LoadMarkup(const char* kmlPath, const char* kssPath) {
    ui::markup::KMLLoader loader(ctx_, ctx_.GetTheme());
    std::string kss = (kssPath && kssPath[0]) ? kssPath : "";
    int rootIdx = loader.LoadFromFile(kmlPath, kss);
    if (rootIdx >= 0) {
        ctx_.SetParent(rootIdx, ctx_.Root());
    } else {
        for (const auto& err : loader.Errors()) {
            fprintf(stderr, "[KML] Line %d: %s\n", err.line, err.message.c_str());
        }
    }
    return rootIdx;
}

/// Load a UI layout from KML/KSS strings (in-memory).
int UI::LoadMarkupString(const char* kml, const char* kss) {
    ui::markup::KMLLoader loader(ctx_, ctx_.GetTheme());
    std::string kssStr = (kss && kss[0]) ? kss : "";
    int rootIdx = loader.LoadFromString(kml, kssStr);
    if (rootIdx >= 0) {
        ctx_.SetParent(rootIdx, ctx_.Root());
    } else {
        for (const auto& err : loader.Errors()) {
            fprintf(stderr, "[KML] Line %d: %s\n", err.line, err.message.c_str());
        }
    }
    return rootIdx;
}

// ============================================================================
// Input Events
// ============================================================================

/// Forward a pointer-down event to the UI.
void UI::HandlePointerDown(float x, float y, uint8_t button) {
    ui::Event ev;
    ev.type = ui::EventType::PointerDown;
    ev.pointerX = x;
    ev.pointerY = y;
    ev.pointerButton = button;
    ctx_.ProcessEvent(ev);
}

/// Forward a pointer-up event to the UI.
void UI::HandlePointerUp(float x, float y, uint8_t button) {
    ui::Event ev;
    ev.type = ui::EventType::PointerUp;
    ev.pointerX = x;
    ev.pointerY = y;
    ev.pointerButton = button;
    ctx_.ProcessEvent(ev);
}

/// Forward a pointer-move event to the UI.
void UI::HandlePointerMove(float x, float y) {
    ui::Event ev;
    ev.type = ui::EventType::PointerMove;
    ev.pointerX = x;
    ev.pointerY = y;
    ctx_.ProcessEvent(ev);
}

/// Get the cursor type requested by the hovered widget.
ui::CursorType UI::GetRequestedCursor() const {
    return ctx_.GetRequestedCursor();
}

/// Forward a scroll event to the UI.
void UI::HandleScroll(float x, float y, float delta) {
    ui::Event ev;
    ev.type = ui::EventType::Scroll;
    ev.pointerX = x;
    ev.pointerY = y;
    ev.scrollDelta = delta;
    ctx_.ProcessEvent(ev);
}

/// Forward a key-down event to the UI.
void UI::HandleKeyDown(int keyCode, bool shift, bool ctrl, bool alt) {
    // Convert koilo::KeyCode (SDL scancode values) to ui::KeyCode (UI enum)
    ui::KeyCode uiKey = ui::KeyCode::None;
    auto kc = static_cast<koilo::KeyCode>(keyCode);
    switch (kc) {
        case koilo::KeyCode::Tab:       uiKey = ui::KeyCode::Tab;       break;
        case koilo::KeyCode::Escape:    uiKey = ui::KeyCode::Escape;    break;
        case koilo::KeyCode::Return:    uiKey = ui::KeyCode::Return;    break;
        case koilo::KeyCode::Backspace: uiKey = ui::KeyCode::Backspace; break;
        case koilo::KeyCode::Delete:    uiKey = ui::KeyCode::Delete;    break;
        case koilo::KeyCode::Left:      uiKey = ui::KeyCode::Left;      break;
        case koilo::KeyCode::Right:     uiKey = ui::KeyCode::Right;     break;
        case koilo::KeyCode::Up:        uiKey = ui::KeyCode::Up;        break;
        case koilo::KeyCode::Down:      uiKey = ui::KeyCode::Down;      break;
        case koilo::KeyCode::Home:      uiKey = ui::KeyCode::Home;      break;
        case koilo::KeyCode::End:       uiKey = ui::KeyCode::End;       break;
        case koilo::KeyCode::PageUp:    uiKey = ui::KeyCode::PageUp;    break;
        case koilo::KeyCode::PageDown:  uiKey = ui::KeyCode::PageDown;  break;
        case koilo::KeyCode::Space:     uiKey = ui::KeyCode::Space;     break;
        case koilo::KeyCode::Minus:     uiKey = ui::KeyCode::Minus;     break;
        case koilo::KeyCode::Period:    uiKey = ui::KeyCode::Period;    break;
        case koilo::KeyCode::Comma:     uiKey = ui::KeyCode::Comma;     break;
        case koilo::KeyCode::A: uiKey = ui::KeyCode::A; break;
        case koilo::KeyCode::B: uiKey = ui::KeyCode::B; break;
        case koilo::KeyCode::C: uiKey = ui::KeyCode::C; break;
        case koilo::KeyCode::D: uiKey = ui::KeyCode::D; break;
        case koilo::KeyCode::E: uiKey = ui::KeyCode::E; break;
        case koilo::KeyCode::F: uiKey = ui::KeyCode::F; break;
        case koilo::KeyCode::G: uiKey = ui::KeyCode::G; break;
        case koilo::KeyCode::H: uiKey = ui::KeyCode::H; break;
        case koilo::KeyCode::I: uiKey = ui::KeyCode::I; break;
        case koilo::KeyCode::J: uiKey = ui::KeyCode::J; break;
        case koilo::KeyCode::K: uiKey = ui::KeyCode::K; break;
        case koilo::KeyCode::L: uiKey = ui::KeyCode::L; break;
        case koilo::KeyCode::M: uiKey = ui::KeyCode::M; break;
        case koilo::KeyCode::N: uiKey = ui::KeyCode::N; break;
        case koilo::KeyCode::O: uiKey = ui::KeyCode::O; break;
        case koilo::KeyCode::P: uiKey = ui::KeyCode::P; break;
        case koilo::KeyCode::Q: uiKey = ui::KeyCode::Q; break;
        case koilo::KeyCode::R: uiKey = ui::KeyCode::R; break;
        case koilo::KeyCode::S: uiKey = ui::KeyCode::S; break;
        case koilo::KeyCode::T: uiKey = ui::KeyCode::T; break;
        case koilo::KeyCode::U: uiKey = ui::KeyCode::U; break;
        case koilo::KeyCode::V: uiKey = ui::KeyCode::V; break;
        case koilo::KeyCode::W: uiKey = ui::KeyCode::W; break;
        case koilo::KeyCode::X: uiKey = ui::KeyCode::X; break;
        case koilo::KeyCode::Y: uiKey = ui::KeyCode::Y; break;
        case koilo::KeyCode::Z: uiKey = ui::KeyCode::Z; break;
        case koilo::KeyCode::Num0: uiKey = ui::KeyCode::Num0; break;
        case koilo::KeyCode::Num1: uiKey = ui::KeyCode::Num1; break;
        case koilo::KeyCode::Num2: uiKey = ui::KeyCode::Num2; break;
        case koilo::KeyCode::Num3: uiKey = ui::KeyCode::Num3; break;
        case koilo::KeyCode::Num4: uiKey = ui::KeyCode::Num4; break;
        case koilo::KeyCode::Num5: uiKey = ui::KeyCode::Num5; break;
        case koilo::KeyCode::Num6: uiKey = ui::KeyCode::Num6; break;
        case koilo::KeyCode::Num7: uiKey = ui::KeyCode::Num7; break;
        case koilo::KeyCode::Num8: uiKey = ui::KeyCode::Num8; break;
        case koilo::KeyCode::Num9: uiKey = ui::KeyCode::Num9; break;
        case koilo::KeyCode::F1:  uiKey = ui::KeyCode::F1;  break;
        case koilo::KeyCode::F2:  uiKey = ui::KeyCode::F2;  break;
        case koilo::KeyCode::F3:  uiKey = ui::KeyCode::F3;  break;
        case koilo::KeyCode::F4:  uiKey = ui::KeyCode::F4;  break;
        case koilo::KeyCode::F5:  uiKey = ui::KeyCode::F5;  break;
        case koilo::KeyCode::F6:  uiKey = ui::KeyCode::F6;  break;
        case koilo::KeyCode::F7:  uiKey = ui::KeyCode::F7;  break;
        case koilo::KeyCode::F8:  uiKey = ui::KeyCode::F8;  break;
        case koilo::KeyCode::F9:  uiKey = ui::KeyCode::F9;  break;
        case koilo::KeyCode::F10: uiKey = ui::KeyCode::F10; break;
        case koilo::KeyCode::F11: uiKey = ui::KeyCode::F11; break;
        case koilo::KeyCode::F12: uiKey = ui::KeyCode::F12; break;
        default: break;
    }
    if (uiKey == ui::KeyCode::None) return;

    ui::Event ev;
    ev.type = ui::EventType::KeyDown;
    ev.key = uiKey;
    ev.mods.shift = shift;
    ev.mods.ctrl = ctrl;
    ev.mods.alt = alt;
    ctx_.ProcessEvent(ev);
}

/// Forward text input to the UI.
void UI::HandleTextInput(const char* text) {
    if (!text || text[0] == '\0') return;
    ui::Event ev;
    ev.type = ui::EventType::TextInput;
    size_t len = std::strlen(text);
    if (len > 7) len = 7;
    std::memcpy(ev.textInput, text, len);
    ev.textInput[len] = '\0';
    ctx_.ProcessEvent(ev);
}

// ============================================================================
// Animation
// ============================================================================

/// Parse a tween property name string to enum.
static ui::TweenProperty ParseTweenProperty(const char* prop) {
    if (!prop) return ui::TweenProperty::PositionX;
    if (strcmp(prop, "x") == 0)       return ui::TweenProperty::PositionX;
    if (strcmp(prop, "y") == 0)       return ui::TweenProperty::PositionY;
    if (strcmp(prop, "width") == 0)   return ui::TweenProperty::Width;
    if (strcmp(prop, "height") == 0)  return ui::TweenProperty::Height;
    if (strcmp(prop, "opacity") == 0) return ui::TweenProperty::Opacity;
    if (strcmp(prop, "slider") == 0)  return ui::TweenProperty::SliderValue;
    if (strcmp(prop, "paddingTop") == 0)    return ui::TweenProperty::PaddingTop;
    if (strcmp(prop, "paddingRight") == 0)  return ui::TweenProperty::PaddingRight;
    if (strcmp(prop, "paddingBottom") == 0) return ui::TweenProperty::PaddingBottom;
    if (strcmp(prop, "paddingLeft") == 0)   return ui::TweenProperty::PaddingLeft;
    return ui::TweenProperty::PositionX;
}

/// Parse an easing type name string to enum.
static ui::EaseType ParseEaseType(const char* ease) {
    if (!ease) return ui::EaseType::Linear;
    if (strcmp(ease, "linear") == 0)     return ui::EaseType::Linear;
    if (strcmp(ease, "easeIn") == 0)     return ui::EaseType::EaseInQuad;
    if (strcmp(ease, "easeOut") == 0)    return ui::EaseType::EaseOutQuad;
    if (strcmp(ease, "easeInOut") == 0)  return ui::EaseType::EaseInOutQuad;
    if (strcmp(ease, "easeInCubic") == 0)    return ui::EaseType::EaseInCubic;
    if (strcmp(ease, "easeOutCubic") == 0)   return ui::EaseType::EaseOutCubic;
    if (strcmp(ease, "easeInOutCubic") == 0) return ui::EaseType::EaseInOutCubic;
    if (strcmp(ease, "elastic") == 0)    return ui::EaseType::EaseInElastic;
    if (strcmp(ease, "elasticOut") == 0) return ui::EaseType::EaseOutElastic;
    if (strcmp(ease, "bounce") == 0)     return ui::EaseType::EaseOutBounce;
    return ui::EaseType::Linear;
}

/// Apply a tween value to the target widget property.
void UI::ApplyTween(int widgetIdx, ui::TweenProperty prop, float value, void* userData) {
    auto* self = static_cast<UI*>(userData);
    ui::Widget* w = self->ctx_.GetWidget(widgetIdx);
    if (!w) return;

    switch (prop) {
        case ui::TweenProperty::PositionX:     w->localX = value; break;
        case ui::TweenProperty::PositionY:     w->localY = value; break;
        case ui::TweenProperty::Width:         w->localW = value; break;
        case ui::TweenProperty::Height:        w->localH = value; break;
        case ui::TweenProperty::Opacity:       break; // TODO: add opacity to widget
        case ui::TweenProperty::SliderValue:   w->sliderValue = value; break;
        case ui::TweenProperty::PaddingTop:    w->padding.top = value; break;
        case ui::TweenProperty::PaddingRight:  w->padding.right = value; break;
        case ui::TweenProperty::PaddingBottom: w->padding.bottom = value; break;
        case ui::TweenProperty::PaddingLeft:   w->padding.left = value; break;
        default: break;
    }
    w->flags.dirty = 1;
}

/// Start a tween animation.
int UI::Animate(int widgetIdx, const char* property, float from, float to,
                float duration, const char* easing) {
    return tweenPool_.Start(widgetIdx, ParseTweenProperty(property), from, to,
                            duration, ParseEaseType(easing));
}

/// Cancel a tween by index.
void UI::CancelAnimation(int tweenIdx) { tweenPool_.Cancel(tweenIdx); }
/// Cancel all tweens on a widget.
void UI::CancelAllAnimations(int widgetIdx) { tweenPool_.CancelAll(widgetIdx); }
/// Update all active animations.
void UI::UpdateAnimations(float dt) { tweenPool_.Update(dt, ApplyTween, this); }

// ============================================================================
// Localization
// ============================================================================

/// Set active locale string.
void UI::SetLocale(const char* locale) { localization_.SetLocale(locale); }

/// Look up a localized string by key.
const char* UI::L(const char* key) { return localization_.Get(key); }

/// Add a localized string entry.
void UI::SetLocalizedString(const char* key, const char* value) {
    localization_.Set(key, value);
}

// ============================================================================
// Accessibility
// ============================================================================

/// Set screen reader label for a widget.
void UI::SetAriaLabel(int idx, const char* label) {
    ui::Widget* w = ctx_.GetWidget(idx);
    if (!w || !label) return;
    w->ariaLabel = ctx_.Strings().Intern(label);
}

/// Get screen reader label for a widget.
const char* UI::GetAriaLabel(int idx) {
    const ui::Widget* w = ctx_.GetWidget(idx);
    if (!w || w->ariaLabel == ui::NullStringId) return "";
    return ctx_.Strings().Lookup(w->ariaLabel);
}

/// Set global font scale multiplier.
void UI::SetFontScale(float scale) { localization_.SetFontScale(scale); }
/// Get global font scale multiplier.
float UI::GetFontScale() { return localization_.FontScale(); }

/// Check if the UI is idle (no pending visual changes).
bool UI::IsIdle() const {
    return !ctx_.IsRenderDirty() && !ctx_.GetTheme().HasActiveTransitions();
}

// ============================================================================
// Reflection Registration
// ============================================================================

KL_DEFINE_FIELDS(UI)
KL_END_FIELDS

KL_DEFINE_METHODS(UI)
    // Viewport & layout
    KL_METHOD_AUTO(UI, SetViewport, "Set viewport dimensions"),
    KL_METHOD_AUTO(UI, UpdateLayout, "Run the layout pass"),
    // Widget creation
    KL_METHOD_AUTO(UI, CreatePanel, "Create a panel widget"),
    KL_METHOD_OVLD(UI, CreateLabel, int, const char*, const char*),
    KL_METHOD_OVLD(UI, CreateButton, int, const char*, const char*),
    KL_METHOD_OVLD(UI, CreateSlider, int, const char*, float, float, float),
    KL_METHOD_OVLD(UI, CreateCheckbox, int, const char*, bool),
    KL_METHOD_OVLD(UI, CreateTextField, int, const char*, const char*),
    KL_METHOD_AUTO(UI, CreateSeparator, "Create a separator"),
    KL_METHOD_AUTO(UI, CreateScrollView, "Create a scroll view"),
    KL_METHOD_OVLD(UI, CreateTreeNode, int, const char*, const char*),
    KL_METHOD_AUTO(UI, CreateDockContainer, "Create a dock container"),
    KL_METHOD_OVLD(UI, CreateSplitPane, int, const char*, bool),
    KL_METHOD_AUTO(UI, CreateTabBar, "Create a tab bar"),
    KL_METHOD_AUTO(UI, CreatePopupMenu, "Create a popup menu"),
    KL_METHOD_OVLD(UI, CreateMenuItem, int, const char*, const char*),
    // Widget tree
    KL_METHOD_OVLD(UI, SetParent, bool, int, int),
    KL_METHOD_OVLD(UI, DestroyWidget, void, int),
    KL_METHOD_AUTO(UI, FindWidget, "Find widget by ID"),
    KL_METHOD_AUTO(UI, GetRoot, "Get root widget index"),
    KL_METHOD_AUTO(UI, GetWidgetCount, "Get number of live widgets"),
    // Properties
    KL_METHOD_OVLD(UI, SetText, void, int, const char*),
    KL_METHOD_OVLD(UI, GetText, const char*, int),
    KL_METHOD_OVLD(UI, SetVisible, void, int, bool),
    KL_METHOD_OVLD(UI, SetEnabled, void, int, bool),
    KL_METHOD_OVLD(UI, SetSelected, void, int, bool),
    KL_METHOD_OVLD(UI, SetColorHex, void, int, const char*),
    KL_METHOD_OVLD(UI, SetSize, void, int, float, float),
    KL_METHOD_OVLD(UI, SetPosition, void, int, float, float),
    KL_METHOD_OVLD(UI, SetPadding, void, int, float, float, float, float),
    KL_METHOD_OVLD(UI, SetMargin, void, int, float, float, float, float),
    // Layout
    KL_METHOD_OVLD(UI, SetLayoutColumn, void, int, float),
    KL_METHOD_OVLD(UI, SetLayoutRow, void, int, float),
    KL_METHOD_OVLD(UI, SetFillWidth, void, int),
    KL_METHOD_OVLD(UI, SetFillHeight, void, int),
    // Events
    KL_METHOD_OVLD(UI, SetOnClick, void, int, const char*),
    KL_METHOD_OVLD(UI, SetOnChange, void, int, const char*),
    // Inspector
    KL_METHOD_OVLD(UI, InspectType, int, const char*, int),
    // Theme
    KL_METHOD_OVLD(UI, SetThemeColor, void, const char*, int, int, int, int),
    // Query
    KL_METHOD_OVLD(UI, GetSliderValue, float, int),
    KL_METHOD_OVLD(UI, GetChecked, bool, int),
    KL_METHOD_AUTO(UI, GetViewportWidth, "Get viewport width"),
    KL_METHOD_AUTO(UI, GetViewportHeight, "Get viewport height"),
    KL_METHOD_AUTO(UI, Clear, "Remove all widgets and reset"),
    // Animation
    KL_METHOD_OVLD(UI, Animate, int, int, const char*, float, float, float, const char*),
    KL_METHOD_OVLD(UI, CancelAnimation, void, int),
    KL_METHOD_OVLD(UI, CancelAllAnimations, void, int),
    KL_METHOD_OVLD(UI, UpdateAnimations, void, float),
    // Localization
    KL_METHOD_OVLD(UI, SetLocale, void, const char*),
    KL_METHOD_AUTO(UI, L, "Look up localized string"),
    KL_METHOD_OVLD(UI, SetLocalizedString, void, const char*, const char*),
    // Accessibility
    KL_METHOD_OVLD(UI, SetAriaLabel, void, int, const char*),
    KL_METHOD_OVLD(UI, GetAriaLabel, const char*, int),
    KL_METHOD_OVLD(UI, SetFontScale, void, float),
    KL_METHOD_AUTO(UI, GetFontScale, "Get global font scale"),

    KL_METHOD_OVLD(UI, LoadMarkup, int, const char*, const char*),
    KL_METHOD_OVLD(UI, LoadMarkupString, int, const char*, const char*)
KL_END_METHODS

KL_DEFINE_DESCRIBE(UI)
    KL_CTOR0(UI)
KL_END_DESCRIBE(UI)

} // namespace koilo
