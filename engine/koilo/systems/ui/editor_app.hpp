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
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/runtime_context.hpp>
#include <koilo/systems/ui/undo_stack.hpp>
#include <koilo/systems/ui/selection.hpp>
#include <koilo/systems/ui/log_buffer.hpp>
#include <koilo/systems/ui/command_registry.hpp>
#include <koilo/systems/ui/color_picker.hpp>
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
    /** @brief Construct the editor application. */
    EditorApp();
    /** @brief Destructor. */
    ~EditorApp();

    /// Build the full editor layout. Call once at startup.
    void Setup(int viewportW, int viewportH);

    /// Per-frame update. Returns true if the UI needs a re-render.
    bool Update(float dt);

    /** @brief Get the UI instance. */
    UI& GetUI() { return ui_; }
    /** @brief Get the undo stack. */
    UndoStack& GetUndoStack() { return undoStack_; }
    /** @brief Get the selection manager. */
    SelectionManager& GetSelection() { return selection_; }
    /** @brief Get the log buffer. */
    LogBuffer& GetLog() { return log_; }
    /** @brief Get the editor runtime context. */
    ui::RuntimeContext& GetContext() { return context_; }
    /** @brief Get the command registry. */
    ui::CommandRegistry& GetCommands() { return commands_; }

    /** @brief Get the menu bar widget index. */
    int GetMenuBar() const { return menuBar_; }
    /** @brief Get the status bar widget index. */
    int GetStatusBar() const { return statusBar_; }
    /** @brief Get the dock area widget index. */
    int GetDockArea() const { return dockArea_; }
    /** @brief Get the left pane widget index. */
    int GetLeftPane() const { return leftPane_; }
    /** @brief Get the center pane widget index. */
    int GetCenterPane() const { return centerPane_; }
    /** @brief Get the right pane widget index. */
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
    void RegisterDefaultCommands();

    // Shared state
    UI ui_;                                                    ///< UI engine instance.
    ui::RuntimeContext context_{ui::ContextRole::Editor};       ///< Editor runtime context.
    ui::CommandRegistry commands_;                              ///< Registered editor commands.
    ui::ColorPicker colorPicker_;                               ///< Color picker popup widget.
    UndoStack undoStack_;                                       ///< Undo/redo history stack.
    SelectionManager selection_;                                ///< Selected objects tracker.
    LogBuffer log_;                                             ///< Console log message buffer.

    // Widget indices for the shell layout
    int menuBar_       = -1;  ///< Menu bar widget index.
    int statusBar_     = -1;  ///< Status bar widget index.
    int dockArea_      = -1;  ///< Dock area widget index.
    int leftPane_      = -1;  ///< Left pane widget index.
    int centerPane_    = -1;  ///< Center pane widget index.
    int rightPane_     = -1;  ///< Right pane widget index.

    // Status bar labels
    int fpsLabel_      = -1;  ///< FPS counter label index.
    int memLabel_      = -1;  ///< Memory usage label index.
    int toolLabel_     = -1;  ///< Active tool label index.

    // Hierarchy panel
    int hierSearch_    = -1;  ///< Hierarchy search field index.
    int hierTree_      = -1;  ///< Hierarchy tree container index.
    int hierCtxMenu_   = -1;  ///< Hierarchy context menu index.

    // Inspector panel
    int inspHeader_    = -1;  ///< Inspector header label index.
    int inspScroll_    = -1;  ///< Inspector scroll view index.
    int inspNoSel_     = -1;  ///< "No selection" label index.

    // Viewport panel
    int vpToolbar_     = -1;  ///< Viewport toolbar panel index.
    int vpView_        = -1;  ///< Viewport render area index.

    // Console panel
    int consScroll_    = -1;  ///< Console scroll view index.
    int consLog_       = -1;  ///< Console log container index.
    int consInput_     = -1;  ///< Console input field index.

    // Asset browser
    int abTree_        = -1;    ///< Asset tree view index.
    int abPreview_     = -1;    ///< Asset preview area index.
    int abPath_        = -1;    ///< Asset path label index.

    float fpsTimer_    = 0.0f;  ///< Accumulated time for FPS counter.
    int frameCount_    = 0;     ///< Frame counter for FPS calculation.

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
