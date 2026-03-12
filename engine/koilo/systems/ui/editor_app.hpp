// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file editor_app.hpp
 * @brief Editor application built on top of the UI engine.
 *
 * EditorApp creates the editor UIContext, sets up the docking layout,
 * and instantiates all editor panels (hierarchy, inspector, viewport,
 * console, asset browser). The editor is built using the same widget
 * API available to game scripts - proving full feature parity.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/runtime_context.hpp>
#include <koilo/systems/ui/undo_stack.hpp>
#include <koilo/systems/ui/selection.hpp>
#include <koilo/systems/ui/log_buffer.hpp>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class EditorApp
 * @brief Coordinates the editor panels and shared state.
 *
 * Owns the editor RuntimeContext, UndoStack, SelectionManager, and LogBuffer.
 * Panels are built via the UI class (same API exposed to KoiloScript).
 */
class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    /// Build the full editor layout. Call once at startup.
    void Setup(int viewportW, int viewportH);

    /// Per-frame update. Returns true if the UI needs a re-render.
    bool Update(float dt);

    /// Accessors for shared editor state
    UI& GetUI() { return ui_; }
    UndoStack& GetUndoStack() { return undoStack_; }
    SelectionManager& GetSelection() { return selection_; }
    LogBuffer& GetLog() { return log_; }
    ui::RuntimeContext& GetContext() { return context_; }

    // Panel root widget indices (for external queries)
    int GetMenuBar() const { return menuBar_; }
    int GetStatusBar() const { return statusBar_; }
    int GetDockArea() const { return dockArea_; }
    int GetLeftPane() const { return leftPane_; }
    int GetCenterPane() const { return centerPane_; }
    int GetRightPane() const { return rightPane_; }

private:
    // Build individual panels
    void BuildMenuBar();
    void BuildStatusBar();
    void BuildHierarchyPanel();
    void BuildInspectorPanel();
    void BuildViewportPanel();
    void BuildConsolePanel();
    void BuildAssetBrowserPanel();

    // Shared state
    UI ui_;
    ui::RuntimeContext context_{ui::ContextRole::Editor};
    UndoStack undoStack_;
    SelectionManager selection_;
    LogBuffer log_;

    // Widget indices for the shell layout
    int menuBar_       = -1;
    int statusBar_     = -1;
    int dockArea_      = -1;
    int leftPane_      = -1;
    int centerPane_    = -1;
    int rightPane_     = -1;

    // Status bar labels
    int fpsLabel_      = -1;
    int memLabel_      = -1;
    int toolLabel_     = -1;

    // Hierarchy panel
    int hierSearch_    = -1;
    int hierTree_      = -1;
    int hierCtxMenu_   = -1;

    // Inspector panel
    int inspHeader_    = -1;
    int inspScroll_    = -1;
    int inspNoSel_     = -1;

    // Viewport panel
    int vpToolbar_     = -1;
    int vpView_        = -1;

    // Console panel
    int consScroll_    = -1;
    int consLog_       = -1;
    int consInput_     = -1;

    // Asset browser
    int abTree_        = -1;
    int abPreview_     = -1;
    int abPath_        = -1;

    float fpsTimer_    = 0.0f;
    int frameCount_    = 0;

    KL_BEGIN_FIELDS(EditorApp)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(EditorApp)
        KL_METHOD_AUTO(EditorApp, GetUndoStack, "Get undo stack"),
        KL_METHOD_AUTO(EditorApp, GetSelection, "Get selection"),
        KL_METHOD_AUTO(EditorApp, GetLog, "Get log"),
        KL_METHOD_AUTO(EditorApp, GetContext, "Get context"),
        KL_METHOD_AUTO(EditorApp, GetStatusBar, "Get status bar"),
        KL_METHOD_AUTO(EditorApp, GetDockArea, "Get dock area"),
        KL_METHOD_AUTO(EditorApp, GetLeftPane, "Get left pane"),
        KL_METHOD_AUTO(EditorApp, GetCenterPane, "Get center pane"),
        KL_METHOD_AUTO(EditorApp, GetRightPane, "Get right pane")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(EditorApp)
        KL_CTOR0(EditorApp)
    KL_END_DESCRIBE(EditorApp)

};

} // namespace koilo
