// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file editor_app.cpp
 * @brief Editor application implementation.
 *
 * Builds the full editor widget tree using the UI class - the same API
 * available to game scripts via reflection.
 *
 * @date 03/09/2026
 * @author Coela Can't
 */

#include "editor_app.hpp"
#include <cstdio>

namespace koilo {

/// Construct the editor and log initialization.
EditorApp::EditorApp() {
    log_.Push(LogSeverity::Info, "Editor initialized");
}

/// Destructor.
EditorApp::~EditorApp() = default;

/// Build the full editor layout.
void EditorApp::Setup(int viewportW, int viewportH) {
    ui_.SetViewport(viewportW, viewportH);

    int root = ui_.GetRoot();
    ui_.SetLayoutColumn(root, 0);

    // -- Menu bar ----------------------------------------------------
    BuildMenuBar();

    // -- Dock area (main split) --------------------------------------
    dockArea_ = ui_.CreateDockContainer("dockArea");
    ui_.SetParent(dockArea_, root);

    // Left pane - hierarchy
    leftPane_ = ui_.CreateSplitPane("leftPane", true);
    ui_.SetParent(leftPane_, dockArea_);
    ui_.SetSize(leftPane_, 250, 0);
    ui_.SetFillHeight(leftPane_);
    BuildHierarchyPanel();

    // Center pane - viewport on top, console on bottom
    centerPane_ = ui_.CreateSplitPane("centerPane", true);
    ui_.SetParent(centerPane_, dockArea_);
    ui_.SetFillWidth(centerPane_);
    ui_.SetFillHeight(centerPane_);
    BuildViewportPanel();
    BuildConsolePanel();

    // Right pane - inspector
    rightPane_ = ui_.CreateSplitPane("rightPane", true);
    ui_.SetParent(rightPane_, dockArea_);
    ui_.SetSize(rightPane_, 300, 0);
    ui_.SetFillHeight(rightPane_);
    BuildInspectorPanel();

    // -- Status bar --------------------------------------------------
    BuildStatusBar();

    // Wire command registry into the UI context
    context_.UI().SetCommandRegistry(&commands_);
    RegisterDefaultCommands();

    // Build color picker popup
    colorPicker_.Build(context_.UI());

    log_.Push(LogSeverity::Info, "Editor layout built");
}

// ============================================================================
// Menu Bar
// ============================================================================

/// Build the menu bar with File, Edit, View, Tools, Help buttons.
void EditorApp::BuildMenuBar() {
    int root = ui_.GetRoot();
    menuBar_ = ui_.CreatePanel("menuBar");
    ui_.SetParent(menuBar_, root);
    ui_.SetFillWidth(menuBar_);
    ui_.SetSize(menuBar_, 0, 28);
    ui_.SetLayoutRow(menuBar_, 4);
    ui_.SetPadding(menuBar_, 2, 8, 2, 8);

    auto addMenuBtn = [&](const char* id, const char* label, int w) {
        int btn = ui_.CreateButton(id, label);
        ui_.SetParent(btn, menuBar_);
        ui_.SetSize(btn, w, 24);
        return btn;
    };

    addMenuBtn("menuFile",  "File",  48);
    addMenuBtn("menuEdit",  "Edit",  48);
    addMenuBtn("menuView",  "View",  48);
    addMenuBtn("menuTools", "Tools", 52);
    addMenuBtn("menuHelp",  "Help",  48);
}

// ============================================================================
// Status Bar
// ============================================================================

/// Build the status bar with FPS, memory, and tool labels.
void EditorApp::BuildStatusBar() {
    int root = ui_.GetRoot();
    statusBar_ = ui_.CreatePanel("statusBar");
    ui_.SetParent(statusBar_, root);
    ui_.SetFillWidth(statusBar_);
    ui_.SetSize(statusBar_, 0, 22);
    ui_.SetLayoutRow(statusBar_, 8);
    ui_.SetPadding(statusBar_, 2, 8, 2, 8);

    fpsLabel_ = ui_.CreateLabel("statusFps", "FPS: ---");
    ui_.SetParent(fpsLabel_, statusBar_);
    ui_.SetSize(fpsLabel_, 80, 18);

    memLabel_ = ui_.CreateLabel("statusMem", "Mem: ---");
    ui_.SetParent(memLabel_, statusBar_);
    ui_.SetSize(memLabel_, 100, 18);

    toolLabel_ = ui_.CreateLabel("statusTool", "Select");
    ui_.SetParent(toolLabel_, statusBar_);
    ui_.SetSize(toolLabel_, 80, 18);
}

// ============================================================================
// Hierarchy Panel
// ============================================================================

/// Build the hierarchy panel with search, tree, and context menu.
void EditorApp::BuildHierarchyPanel() {
    // Header
    int header = ui_.CreatePanel("hierHeader");
    ui_.SetParent(header, leftPane_);
    ui_.SetFillWidth(header);
    ui_.SetSize(header, 0, 28);
    ui_.SetLayoutRow(header, 4);
    ui_.SetPadding(header, 2, 4, 2, 4);

    int title = ui_.CreateLabel("hierTitle", "Hierarchy");
    ui_.SetParent(title, header);
    ui_.SetSize(title, 80, 24);

    hierSearch_ = ui_.CreateTextField("hierSearch", "Search...");
    ui_.SetParent(hierSearch_, header);
    ui_.SetFillWidth(hierSearch_);
    ui_.SetSize(hierSearch_, 0, 24);

    // Scrollable tree area
    int scrollArea = ui_.CreateScrollView("hierScroll");
    ui_.SetParent(scrollArea, leftPane_);
    ui_.SetFillWidth(scrollArea);
    ui_.SetFillHeight(scrollArea);

    hierTree_ = ui_.CreatePanel("hierTree");
    ui_.SetParent(hierTree_, scrollArea);
    ui_.SetFillWidth(hierTree_);
    ui_.SetLayoutColumn(hierTree_, 0);

    // Context menu
    hierCtxMenu_ = ui_.CreatePopupMenu("hierCtxMenu");
    ui_.SetParent(hierCtxMenu_, leftPane_);

    auto addMenuItem = [&](const char* id, const char* label) {
        int item = ui_.CreateMenuItem(id, label);
        ui_.SetParent(item, hierCtxMenu_);
        return item;
    };

    addMenuItem("hierCtxAdd",    "Add Child");
    addMenuItem("hierCtxDup",    "Duplicate");
    addMenuItem("hierCtxRename", "Rename");
    int sep = ui_.CreateSeparator("hierCtxSep");
    ui_.SetParent(sep, hierCtxMenu_);
    addMenuItem("hierCtxDel",    "Delete");
}

// ============================================================================
// Inspector Panel
// ============================================================================

/// Build the inspector panel with header and scrollable content.
void EditorApp::BuildInspectorPanel() {
    // Header
    int header = ui_.CreatePanel("inspHeader");
    ui_.SetParent(header, rightPane_);
    ui_.SetFillWidth(header);
    ui_.SetSize(header, 0, 28);
    ui_.SetLayoutRow(header, 4);
    ui_.SetPadding(header, 2, 4, 2, 4);

    inspHeader_ = ui_.CreateLabel("inspTitle", "Inspector");
    ui_.SetParent(inspHeader_, header);
    ui_.SetFillWidth(inspHeader_);
    ui_.SetSize(inspHeader_, 0, 24);

    // Scrollable content
    inspScroll_ = ui_.CreateScrollView("inspScroll");
    ui_.SetParent(inspScroll_, rightPane_);
    ui_.SetFillWidth(inspScroll_);
    ui_.SetFillHeight(inspScroll_);

    inspNoSel_ = ui_.CreateLabel("inspNoSel", "No object selected");
    ui_.SetParent(inspNoSel_, inspScroll_);
    ui_.SetFillWidth(inspNoSel_);
    ui_.SetSize(inspNoSel_, 0, 24);
}

// ============================================================================
// Viewport Panel
// ============================================================================

/// Build the viewport panel with toolbar and render area.
void EditorApp::BuildViewportPanel() {
    // Toolbar
    vpToolbar_ = ui_.CreatePanel("vpToolbar");
    ui_.SetParent(vpToolbar_, centerPane_);
    ui_.SetFillWidth(vpToolbar_);
    ui_.SetSize(vpToolbar_, 0, 28);
    ui_.SetLayoutRow(vpToolbar_, 2);
    ui_.SetPadding(vpToolbar_, 2, 4, 2, 4);

    auto addToolBtn = [&](const char* id, const char* label, int w) {
        int btn = ui_.CreateButton(id, label);
        ui_.SetParent(btn, vpToolbar_);
        ui_.SetSize(btn, w, 24);
        return btn;
    };

    addToolBtn("vpSelect", "Select", 56);
    addToolBtn("vpMove",   "Move",   48);
    addToolBtn("vpRotate", "Rotate", 56);
    addToolBtn("vpScale",  "Scale",  48);

    // Viewport area
    vpView_ = ui_.CreatePanel("vpView");
    ui_.SetParent(vpView_, centerPane_);
    ui_.SetFillWidth(vpView_);
    ui_.SetFillHeight(vpView_);
}

// ============================================================================
// Console Panel
// ============================================================================

/// Build the console panel with filter buttons, log, and input.
void EditorApp::BuildConsolePanel() {
    // Header
    int header = ui_.CreatePanel("consHeader");
    ui_.SetParent(header, centerPane_);
    ui_.SetFillWidth(header);
    ui_.SetSize(header, 0, 28);
    ui_.SetLayoutRow(header, 4);
    ui_.SetPadding(header, 2, 4, 2, 4);

    int title = ui_.CreateLabel("consTitle", "Console");
    ui_.SetParent(title, header);
    ui_.SetSize(title, 60, 24);

    auto addFilterBtn = [&](const char* id, const char* label, int w) {
        int btn = ui_.CreateButton(id, label);
        ui_.SetParent(btn, header);
        ui_.SetSize(btn, w, 24);
        return btn;
    };

    addFilterBtn("consClear", "Clear", 48);
    addFilterBtn("consInfo",  "Info",  40);
    addFilterBtn("consWarn",  "Warn",  44);
    addFilterBtn("consErr",   "Error", 44);

    // Scroll area
    consScroll_ = ui_.CreateScrollView("consScroll");
    ui_.SetParent(consScroll_, centerPane_);
    ui_.SetFillWidth(consScroll_);
    ui_.SetSize(consScroll_, 0, 150);

    consLog_ = ui_.CreatePanel("consLog");
    ui_.SetParent(consLog_, consScroll_);
    ui_.SetFillWidth(consLog_);
    ui_.SetLayoutColumn(consLog_, 0);

    // Input field
    consInput_ = ui_.CreateTextField("consInput", "Enter command...");
    ui_.SetParent(consInput_, centerPane_);
    ui_.SetFillWidth(consInput_);
    ui_.SetSize(consInput_, 0, 24);
}

// ============================================================================
// Asset Browser
// ============================================================================

/// Build the asset browser panel (placeholder).
void EditorApp::BuildAssetBrowserPanel() {
    // Would be placed in a tab alongside console
    // Omitted from default layout - can be toggled via View menu
}

// ============================================================================
// Update
// ============================================================================

/// Per-frame update: refresh FPS and memory labels.
bool EditorApp::Update(float dt) {
    frameCount_++;
    fpsTimer_ += dt;

    if (fpsTimer_ >= 1.0f) {
        char buf[32];
        snprintf(buf, sizeof(buf), "FPS: %d", frameCount_);
        ui_.SetText(fpsLabel_, buf);

        snprintf(buf, sizeof(buf), "Mem: %zuKB",
                 context_.MemoryUsage() / 1024);
        ui_.SetText(memLabel_, buf);

        frameCount_ = 0;
        fpsTimer_ = 0.0f;
    }

    return true;
}

/// Register built-in editor commands and keyboard shortcuts.
void EditorApp::RegisterDefaultCommands() {
    using S = ui::Shortcut;
    using K = ui::KeyCode;

    commands_.Register("edit.undo", "Undo", "Edit",
        [this]() { undoStack_.Undo(); },
        [this]() { return undoStack_.CanUndo(); },
        S(K::Z, true));

    commands_.Register("edit.redo", "Redo", "Edit",
        [this]() { undoStack_.Redo(); },
        [this]() { return undoStack_.CanRedo(); },
        S(K::Z, true, true));

    commands_.Register("edit.delete", "Delete", "Edit",
        [this]() { selection_.ClearSelection(); },
        nullptr,
        S(K::Delete));

    commands_.Register("edit.selectall", "Select All", "Edit",
        []() { /* placeholder - needs widget tree integration */ },
        nullptr,
        S(K::A, true));

    commands_.Register("view.fullscreen", "Toggle Fullscreen", "View",
        []() { /* placeholder - needs platform integration */ },
        nullptr,
        S(K::F11));
}

} // namespace koilo
