// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_context.hpp
 * @brief UIContext owns a widget tree, theme, event queue, and drives
 *        layout and event dispatch for one isolated UI context.
 *
 * Separate UIContext instances are used for editor and game UI.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "widget.hpp"
#include "layout.hpp"
#include "event.hpp"
#include "style.hpp"
#include "command_registry.hpp"
#include <string>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

class ColorPicker;  // Forward declaration.

/**
 * @class UIContext
 * @brief Owns a widget tree, theme, event queue, layout engine and event dispatch
 *        for one isolated UI context.
 *
 * Separate UIContext instances are used for editor and game UI.
 * Provides widget creation, property access, event processing, popup management,
 * tree-node navigation, selection, drag-and-drop, and floating panel support.
 */
class UIContext {
public:
    /// Callback type for script function invocation.
    using ScriptCallbackFn = std::function<void(const char* fnName)>;

    /** @brief Construct a UIContext with the given widget pool capacity. */
    explicit UIContext(size_t poolCapacity = WidgetPool::DEFAULT_CAPACITY);

    /** @brief Destructor (defined in .cpp for unique_ptr forward-declared type). */
    ~UIContext();

    /// Set the callback used to invoke script functions (e.g. OnClick handlers).
    void SetScriptCallback(ScriptCallbackFn fn) { scriptCallback_ = std::move(fn); }

    /// Attach a command registry for keyboard shortcut dispatch.
    void SetCommandRegistry(CommandRegistry* reg) { commands_ = reg; }
    /** @brief Get the attached command registry. */
    CommandRegistry* GetCommandRegistry() const { return commands_; }

    // ---- Widget Tree Management ----

    /// Create a widget and return its pool index. Returns -1 on failure.
    int CreateWidget(WidgetTag tag, const char* id);

    /** @brief Create a panel widget. */
    int CreatePanel(const char* id)  { return CreateWidget(WidgetTag::Panel, id); }
    /** @brief Create a label widget. */
    int CreateLabel(const char* id, const char* text);
    /** @brief Create a button widget. */
    int CreateButton(const char* id, const char* text);
    /**
     * @brief Create a slider widget.
     * @param id    Widget id.
     * @param min   Minimum slider value.
     * @param max   Maximum slider value.
     * @param value Initial slider value.
     */
    int CreateSlider(const char* id, float min, float max, float value);
    /** @brief Create a checkbox widget. */
    int CreateCheckbox(const char* id, bool checked);
    /** @brief Create a text field widget. */
    int CreateTextField(const char* id, const char* placeholder = "");
    /** @brief Create a separator widget. */
    int CreateSeparator(const char* id);
    /** @brief Create a progress bar widget. */
    int CreateProgressBar(const char* id, float value = 0.0f);
    /** @brief Create a toggle switch widget. */
    int CreateToggleSwitch(const char* id, bool on = false);
    /** @brief Create a radio button widget. */
    int CreateRadioButton(const char* id, const char* group, bool selected = false);
    /**
     * @brief Create a number spinner widget.
     * @param id    Widget id.
     * @param value Initial value.
     * @param min   Minimum value.
     * @param max   Maximum value.
     * @param step  Step increment per click or key press.
     */
    int CreateNumberSpinner(const char* id, float value = 0.0f, float min = 0.0f,
                            float max = 100.0f, float step = 1.0f);

    /// Create a virtual list widget that only renders visible items.
    /// @param id        Widget id
    /// @param itemCount Total number of data items
    /// @param itemH     Height of each item in pixels
    /// @param bind      Callback to populate an item widget for a given index
    int CreateVirtualList(const char* id, int itemCount, float itemH,
                          std::function<void(Widget&, int)> bind);

    /// Update the visible items in a VirtualList based on scroll position.
    void UpdateVirtualList(int listIdx);

    /// Create a Canvas2D widget for custom drawing.
    /// @param id      Widget id
    /// @param onPaint Callback receiving a CanvasDrawContext* (as void*)
    int CreateCanvas2D(const char* id,
                       std::function<void(void*)> paintFn = nullptr);

    /** @brief Create a dock container widget. */
    int CreateDockContainer(const char* id);

    /** @brief Create a split pane widget. */
    int CreateSplitPane(const char* id, bool vertical = true);

    /// Create a floating panel overlay. Attach to root for screen-space positioning.
    int CreateFloatingPanel(const char* id, const char* title,
                            float x, float y, float w, float h);

    /// Undock a panel from its parent and convert it to a floating panel.
    int UndockPanel(int panelIdx, float screenX, float screenY);

    /// Re-dock a floating panel back to its original location.
    bool RedockPanel(int floatIdx);

    /// Bring a floating panel to front (highest z-order).
    void BringToFront(int panelIdx);

    /** @brief Create a tab bar widget. */
    int CreateTabBar(const char* id);

    /** @brief Create a popup menu widget. */
    int CreatePopupMenu(const char* id);

    /** @brief Create a menu item widget. */
    int CreateMenuItem(const char* id, const char* text);

    // ---- Tree Node API ----

    /// Create a tree node and attach it to a parent container.
    int AddTreeNode(int containerIdx, const char* id, const char* text,
                    int depth = 0, bool hasChildren = false);

    /// Remove a tree node and collapse its visual children.
    void RemoveTreeNode(int nodeIdx);

    /// Reparent a tree node to a new position.
    void ReparentTreeNode(int nodeIdx, int containerIdx, int insertAfterIdx, int newDepth);

    /// Recalculate treeHasChildren for all tree nodes in a container.
    void RefreshTreeFlags(int containerIdx);

    /// Enable drag-to-reparent on a TreeNode widget.
    void EnableTreeNodeDragDrop(int nodeIdx);

    /// Enable drag-to-reparent on all TreeNode children in a container.
    void EnableTreeDragDrop(int containerIdx);

    // ---- Selection API ----

    /// Select a single tree node, clearing any previous selection.
    void SelectNode(int nodeIdx);

    /// Toggle selection on a node (for Ctrl+Click).
    void ToggleSelectNode(int nodeIdx);

    /// Range-select tree nodes between anchor and target (for Shift+Click).
    void RangeSelectNodes(int anchorIdx, int targetIdx);

    /// Deselect a specific node.
    void DeselectNode(int nodeIdx);

    /// Clear all selection.
    void ClearSelection();

    /// Get currently selected node indices.
    const std::vector<int>& SelectedNodes() const { return selectedNodes_; }

    /// Selection anchor for Shift+Click range selection.
    int SelectionAnchor() const { return selectionAnchor_; }

    /// Attach child to parent. Returns true on success.
    bool SetParent(int childIdx, int parentIdx);

    /// Destroy a widget and all its children recursively.
    void DestroyWidget(int idx);

    /// Set the root widget index. This widget gets the screen rect.
    void SetRoot(int idx) { rootIndex_ = idx; }
    /** @brief Get the root widget index. */
    int Root() const { return rootIndex_; }

    // ---- Property Access ----

    /** @brief Get a widget by pool index (mutable). */
    Widget* GetWidget(int idx) { return pool_.Get(idx); }
    /** @brief Get a widget by pool index (const). */
    const Widget* GetWidget(int idx) const { return pool_.Get(idx); }

    /** @brief Set the display text of a widget. */
    void SetText(int idx, const char* text) {
        Widget* w = pool_.Get(idx);
        if (w) w->textId = strings_.Intern(text);
    }

    /** @brief Get the display text of a widget. */
    const char* GetText(int idx) const {
        const Widget* w = pool_.Get(idx);
        if (!w) return "";
        return strings_.Lookup(w->textId);
    }

    /** @brief Set visibility of a widget. */
    void SetVisible(int idx, bool v) {
        Widget* w = pool_.Get(idx);
        if (w) { w->flags.visible = v ? 1 : 0; MarkDirty(idx); }
    }

    /** @brief Set the enabled state of a widget. */
    void SetEnabled(int idx, bool e) {
        Widget* w = pool_.Get(idx);
        if (w) w->flags.enabled = e ? 1 : 0;
    }

    /** @brief Set the selected state of a widget. */
    void SetSelected(int idx, bool s) {
        Widget* w = pool_.Get(idx);
        if (w) {
            w->flags.selected = s ? 1 : 0;
            theme_.NotifyStateChange(*w, idx);
            MarkDirty(idx);
        }
    }

    /** @brief Set fixed size on a widget. */
    void SetSize(int idx, float width, float height) {
        Widget* w = pool_.Get(idx);
        if (w) {
            w->localW = width; w->localH = height;
            w->widthMode = SizeMode::Fixed;
            w->heightMode = SizeMode::Fixed;
            MarkDirty(idx);
        }
    }

    /** @brief Set the local position of a widget. */
    void SetPosition(int idx, float x, float y) {
        Widget* w = pool_.Get(idx);
        if (w) { w->localX = x; w->localY = y; MarkDirty(idx); }
    }

    /** @brief Set layout direction, alignment, and spacing on a widget. */
    void SetLayout(int idx, LayoutDirection dir, Alignment mainAlign = Alignment::Start,
                   Alignment crossAlign = Alignment::Start, float spacing = 0.0f);

    /** @brief Set padding on a widget. */
    void SetPadding(int idx, float top, float right, float bottom, float left);

    /** @brief Set margin on a widget. */
    void SetMargin(int idx, float top, float right, float bottom, float left);

    /** @brief Set anchor offsets on a widget. */
    void SetAnchors(int idx, float minX, float minY, float maxX, float maxY);

    /** @brief Set width and height sizing modes on a widget. */
    void SetSizeMode(int idx, SizeMode wMode, SizeMode hMode);

    // ---- Event Callbacks ----

    /** @brief Set a C++ click handler on a widget. */
    void SetOnClick(int idx, std::function<void(Widget&)> fn) {
        Widget* w = pool_.Get(idx);
        if (w) w->onClickCpp = std::move(fn);
    }

    /** @brief Set a C++ change handler on a widget. */
    void SetOnChange(int idx, std::function<void(Widget&)> fn) {
        Widget* w = pool_.Get(idx);
        if (w) w->onChangeCpp = std::move(fn);
    }

    /** @brief Set a script function name for click dispatch. */
    void SetOnClickScript(int idx, const char* fnName) {
        Widget* w = pool_.Get(idx);
        if (w) w->onClickId = strings_.Intern(fnName);
    }

    /** @brief Set a script function name for change dispatch. */
    void SetOnChangeScript(int idx, const char* fnName) {
        Widget* w = pool_.Get(idx);
        if (w) w->onChangeId = strings_.Intern(fnName);
    }

    // ---- Update Cycle ----

    /// Set the viewport size (typically the window size).
    void SetViewport(float width, float height);

    /// Run layout pass. Updates computedRect on all dirty widgets.
    void UpdateLayout();

    /// Process a raw input event. Performs hit testing, focus, and dispatch.
    void ProcessEvent(Event event);

    // ---- Query ----

    /** @brief Get the focused widget index. */
    int FocusedWidget() const { return focusedWidget_; }
    /** @brief Get the hovered widget index. */
    int HoveredWidget() const { return hoveredWidget_; }
    /** @brief Get the viewport width. */
    float ViewportWidth() const { return viewportW_; }
    /** @brief Get the viewport height. */
    float ViewportHeight() const { return viewportH_; }

    /** @brief Check if a drag operation is active. */
    bool IsDragging() const { return activeDrag_.IsActive(); }
    /** @brief Get the active drag payload. */
    const DragPayload& ActiveDrag() const { return activeDrag_; }
    /** @brief Get the widget currently hovered during a drag. */
    int DragHoverTarget() const { return dragHoverTarget_; }
    /** @brief Get the drop position indicator during a drag. */
    DropPosition DragDropPosition() const { return dragDropPos_; }
    /** @brief Get the last pointer X position. */
    float DragPointerX() const { return lastPointerX_; }
    /** @brief Get the last pointer Y position. */
    float DragPointerY() const { return lastPointerY_; }

    /** @brief Get the widget pool (const). */
    const WidgetPool& Pool() const { return pool_; }
    /** @brief Get the widget pool (mutable). */
    WidgetPool& Pool() { return pool_; }
    /** @brief Get the string table (const). */
    const StringTable& Strings() const { return strings_; }
    /** @brief Get the string table (mutable). */
    StringTable& Strings() { return strings_; }
    /** @brief Get the theme (const). */
    const Theme& GetTheme() const { return theme_; }
    /** @brief Get the theme (mutable). */
    Theme& GetTheme() { return theme_; }

    /// Update transition animations. Call once per frame with delta time in seconds.
    void UpdateTransitions(float dt);

    /// Get active tooltip info. Returns widget index or -1 if no tooltip visible.
    int GetTooltipWidget() const { return tooltipWidget_; }
    /** @brief Get tooltip X position. */
    float GetTooltipX() const { return tooltipX_; }
    /** @brief Get tooltip Y position. */
    float GetTooltipY() const { return tooltipY_; }

    /// Get the cursor style for the currently hovered widget.
    CursorType GetRequestedCursor() const;

    /// Find a widget by interned string ID (linear scan).
    int FindWidget(const char* id) const;

    /// Get the pool index of the topmost active popup, or -1.
    int ActivePopup() const { return activePopup_; }

    /** @brief Show a popup at a screen position. */
    void ShowPopup(int popupIdx, float x, float y);

    /// Dismiss the top popup only. Returns true if a popup was dismissed.
    bool DismissTopPopup();

    /// Dismiss all active popups.
    void DismissPopup();

    /// Show a sub-menu popup to the right of a MenuItem.
    void ShowSubMenu(int menuItemIdx, int submenuPopupIdx);

    // -- Render dirty flag ---------------------------------------

    /** @brief True if the UI needs a draw list rebuild (state changed since last render). */
    bool IsRenderDirty() const { return renderDirty_; }

    /** @brief Clear the render dirty flag (call after draw list is built). */
    void ClearRenderDirty() { renderDirty_ = false; }

    /** @brief Force the UI to be marked as needing a redraw. */
    void SetRenderDirty() { renderDirty_ = true; }

private:
    static constexpr int MAX_SMOOTH_SCROLLS = 8; ///< Maximum concurrent smooth scroll animations.

    /**
     * @struct SmoothScroll
     * @brief Tracks a single smooth scroll animation on a widget.
     */
    struct SmoothScroll {
        int widgetIdx = -1;  ///< Pool index of the scrolling widget.
        float targetY = 0.0f; ///< Target scroll offset.
        bool active = false;  ///< Whether this slot is in use.
    };

    void MarkDirty(int idx) {
        Widget* w = pool_.Get(idx);
        if (w) w->flags.dirty = 1;
        renderDirty_ = true;
    }

    SmoothScroll* FindOrCreateSmoothScroll(int widgetIdx);

    void UpdateSmoothScroll(float dt);

    void RemoveChild(Widget& parent, int childIdx);

    void SetFocus(int idx, bool fromKeyboard = false);

    void HandlePointerDown(Event& event);

    void HandlePointerUp(Event& event);

    void HandlePointerMove(Event& event);

    void HandleKeyDown(Event& event);

    void HandleTextInput(Event& event);

    void HandleScroll(Event& event);

    void HandleRightClick(Event& event);

    void UpdateTreeVisibility(const Widget& toggledNode);

    std::vector<int> GetVisibleTreeNodes(int containerIdx) const;

    void ScrollNodeIntoView(int nodeIdx);

    void HandleTreeNavigation(KeyCode key, Modifiers mods);

    void DeselectRadioGroupRecursive(Widget* parent, StringId group, Widget* except);

    void DispatchClick(Widget& w);

    bool CanAcceptDrop(const Widget& target, const DragPayload& payload) const;

    ScriptCallbackFn scriptCallback_;           ///< Callback for invoking script functions.
    CommandRegistry* commands_ = nullptr;       ///< Attached command registry for shortcuts.
    WidgetPool pool_;                           ///< Widget object pool.
    StringTable strings_;                       ///< Interned string table.
    Theme theme_;                               ///< Visual theme and transition state.
    LayoutEngine layout_;                       ///< Layout computation engine.
    FocusManager focus_;                        ///< Tab-order focus manager.

    bool renderDirty_ = true;                   ///< True when UI state changed since last draw list build.

    int rootIndex_ = -1;                        ///< Pool index of the root widget.
    int focusedWidget_ = -1;                    ///< Pool index of the focused widget.
    int hoveredWidget_ = -1;                    ///< Pool index of the hovered widget.

    // ---- Selection state ----
    std::vector<int> selectedNodes_;            ///< Currently selected node indices.
    int selectionAnchor_ = -1;                  ///< Anchor node for shift-click range selection.
    Modifiers lastModifiers_{};                 ///< Modifier keys from last pointer event.

    // ---- Tooltip state ----
    float tooltipHoverTime_ = 0.0f;            ///< Accumulated hover time for tooltip trigger.
    static constexpr float TOOLTIP_DELAY = 0.5f; ///< Seconds before showing tooltip.
    int tooltipWidget_ = -1;                    ///< Widget showing tooltip, or -1.
    float tooltipX_ = 0.0f;                     ///< Tooltip X position.
    float tooltipY_ = 0.0f;                     ///< Tooltip Y position.

    // ---- Context menu / popup state ----
    static constexpr int MAX_POPUP_DEPTH = 8;  ///< Maximum nesting depth for popup menus.
    int popupStack_[MAX_POPUP_DEPTH]{};        ///< Stack of active popup pool indices.
    int popupDepth_ = 0;                       ///< Number of active popups.
    int activePopup_ = -1;                     ///< Top of popup stack (cached for quick check).
    float submenuHoverTime_ = 0.0f;            ///< Timer for hover-to-expand sub-menu.
    int submenuHoverItem_ = -1;                ///< MenuItem being hovered for sub-menu expansion.
    bool menuBarOpen_ = false;                 ///< True when any top-level menu bar popup is open.

    // ---- Smooth scroll state ----
    SmoothScroll smoothScrolls_[MAX_SMOOTH_SCROLLS]{}; ///< Active smooth scroll slots.

    float viewportW_ = 1920.0f;                ///< Current viewport width.
    float viewportH_ = 1080.0f;                ///< Current viewport height.

    // ---- Last pointer position (for click dispatch) ----
    float lastPointerX_ = 0.0f;                ///< Last known pointer X.
    float lastPointerY_ = 0.0f;                ///< Last known pointer Y.

    // ---- Divider resize drag state ----
    static constexpr float DIVIDER_HIT_HALF = 4.0f; ///< Hit region half-width in px.
    static constexpr float MIN_PANEL_SIZE = 40.0f;   ///< Minimum panel size during resize.

    /**
     * @struct ResizeDrag
     * @brief State for an active divider resize drag operation.
     */
    struct ResizeDrag {
        bool active = false;     ///< Whether a resize drag is in progress.
        int parentIdx = -1;      ///< Parent container being resized.
        int childA = -1;         ///< Left/top child index in pool.
        int childB = -1;         ///< Right/bottom child index in pool.
        float startMouse = 0.0f; ///< Mouse position at drag start.
        float startSizeA = 0.0f; ///< childA size at drag start.
        float startSizeB = 0.0f; ///< childB size at drag start.
        bool horizontal = true;  ///< True = dragging X (row), false = dragging Y (col).

        KL_BEGIN_FIELDS(ResizeDrag)
            KL_FIELD(ResizeDrag, active, "Active", 0, 1),
            KL_FIELD(ResizeDrag, parentIdx, "Parent index", -1, 65535),
            KL_FIELD(ResizeDrag, childA, "Child A", -1, 65535),
            KL_FIELD(ResizeDrag, childB, "Child B", -1, 65535),
            KL_FIELD(ResizeDrag, horizontal, "Horizontal", 0, 1)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ResizeDrag)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ResizeDrag)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ResizeDrag)

    } resizeDrag_;

    /**
     * @struct ScrollDrag
     * @brief State for an active scrollbar thumb drag.
     */
    struct ScrollDrag {
        bool active = false;      ///< Whether a scroll drag is in progress.
        int scrollViewIdx = -1;   ///< Pool index of the scroll view being dragged.
        float startMouseY = 0.0f; ///< Mouse Y at drag start.
        float startScrollY = 0.0f; ///< Scroll offset at drag start.
    } scrollDrag_;

    // ---- Floating panel drag state ----
    /// Panel drag mode describing which edge or title bar is being dragged.
    enum class PanelDragMode : uint8_t { None, Move, ResizeLeft, ResizeRight, ResizeTop, ResizeBottom,
                                         ResizeTL, ResizeTR, ResizeBL, ResizeBR };
    /**
     * @struct PanelDrag
     * @brief State for an active floating panel drag/resize operation.
     */
    struct PanelDrag {
        bool active = false;       ///< Whether a panel drag is in progress.
        int panelIdx = -1;         ///< Pool index of the panel being dragged.
        PanelDragMode mode = PanelDragMode::None; ///< Current drag mode.
        float startMouseX = 0.0f;  ///< Mouse X at drag start.
        float startMouseY = 0.0f;  ///< Mouse Y at drag start.
        float startPanelX = 0.0f;  ///< Panel X at drag start.
        float startPanelY = 0.0f;  ///< Panel Y at drag start.
        float startPanelW = 0.0f;  ///< Panel width at drag start.
        float startPanelH = 0.0f;  ///< Panel height at drag start.
    } panelDrag_;

    // ---- General drag-and-drop state ----
    static constexpr float DRAG_THRESHOLD = 4.0f; ///< Pixel movement before drag starts.
    /**
     * @struct DragCandidate
     * @brief Tracks a potential drag source before threshold is reached.
     */
    struct DragCandidate {
        int widgetIdx = -1;   ///< Widget that may start a drag.
        float startX  = 0.0f; ///< Pointer X at press.
        float startY  = 0.0f; ///< Pointer Y at press.
    } dragCandidate_;
    DragPayload activeDrag_;              ///< Current drag payload (typeTag != None while dragging).
    int         dragHoverTarget_ = -1;    ///< Widget currently hovered during drag.
    DropPosition dragDropPos_ = DropPosition::Into; ///< Current drop position indicator.

    // ---- Color picker state ----
    std::unique_ptr<ColorPicker> colorPicker_; ///< Shared color picker popup (lazy-built).

    /// Ensure the color picker is built. Returns the picker instance.
    ColorPicker& EnsureColorPicker();

    /// Check if (px,py) hits a floating panel's title bar or resize edge.
    bool FindFloatingPanelHit(float px, float py);

    void HandlePanelDrag(float px, float py);

    /// Returns the appropriate cursor type if the mouse is over a floating panel edge.
    CursorType GetFloatingPanelEdgeCursor(float px, float py) const;

    /// Minimize a floating panel to the status bar tray.
    void MinimizePanel(int panelIdx);

    /// Restore a minimized floating panel.
    void RestorePanel(int panelIdx);

    /// Get all minimized floating panels (for tray rendering).
    std::vector<int> GetMinimizedPanels() const;

    /// Check if click hit a minimized panel tray button; restore if so.
    bool FindMinimizedTrayHit(float px, float py);

    /// Check if (px,py) is on a divider between children of a layout container.
    bool FindDividerHit(float px, float py);

    void HandleResizeDrag(float px, float py);

    /// Check if (px,py) hits a scrollbar track/thumb on any ScrollView.
    bool FindScrollbarHit(float px, float py);

    void HandleScrollDrag(float py);

    KL_BEGIN_FIELDS(UIContext)
        KL_FIELD(UIContext, rootIndex_, "Root index", -1, 65535),
        KL_FIELD(UIContext, focusedWidget_, "Focused widget", -1, 65535),
        KL_FIELD(UIContext, hoveredWidget_, "Hovered widget", -1, 65535),
        KL_FIELD(UIContext, viewportW_, "Viewport width", 0, 65535),
        KL_FIELD(UIContext, viewportH_, "Viewport height", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIContext)
        KL_METHOD_AUTO(UIContext, CreateWidget, "Create widget"),
        KL_METHOD_AUTO(UIContext, CreatePanel, "Create panel"),
        KL_METHOD_AUTO(UIContext, CreateLabel, "Create label"),
        KL_METHOD_AUTO(UIContext, CreateButton, "Create button"),
        KL_METHOD_AUTO(UIContext, CreateSlider, "Create slider"),
        KL_METHOD_AUTO(UIContext, CreateCheckbox, "Create checkbox"),
        KL_METHOD_AUTO(UIContext, CreateTextField, "Create text field"),
        KL_METHOD_AUTO(UIContext, CreateSeparator, "Create separator"),
        KL_METHOD_AUTO(UIContext, CreateDockContainer, "Create dock container"),
        KL_METHOD_AUTO(UIContext, CreateSplitPane, "Create split pane"),
        KL_METHOD_AUTO(UIContext, CreateTabBar, "Create tab bar"),
        KL_METHOD_AUTO(UIContext, CreatePopupMenu, "Create popup menu"),
        KL_METHOD_AUTO(UIContext, CreateMenuItem, "Create menu item"),
        KL_METHOD_AUTO(UIContext, CreateProgressBar, "Create progress bar"),
        KL_METHOD_AUTO(UIContext, CreateToggleSwitch, "Create toggle switch"),
        KL_METHOD_AUTO(UIContext, CreateRadioButton, "Create radio button"),
        KL_METHOD_AUTO(UIContext, CreateNumberSpinner, "Create number spinner"),
        KL_METHOD_AUTO(UIContext, SetParent, "Set parent"),
        KL_METHOD_AUTO(UIContext, DestroyWidget, "Destroy widget"),
        KL_METHOD_AUTO(UIContext, SetRoot, "Set root"),
        KL_METHOD_AUTO(UIContext, Root, "Root"),
        KL_METHOD_AUTO(UIContext, SetText, "Set text"),
        KL_METHOD_AUTO(UIContext, GetText, "Get text"),
        KL_METHOD_AUTO(UIContext, SetVisible, "Set visible"),
        KL_METHOD_AUTO(UIContext, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(UIContext, SetSelected, "Set selected state"),
        KL_METHOD_AUTO(UIContext, SetSize, "Set size"),
        KL_METHOD_AUTO(UIContext, SetPosition, "Set position"),
        KL_METHOD_AUTO(UIContext, SetLayout, "Set layout"),
        KL_METHOD_AUTO(UIContext, SetPadding, "Set padding"),
        KL_METHOD_AUTO(UIContext, SetMargin, "Set margin"),
        KL_METHOD_AUTO(UIContext, SetAnchors, "Set anchors"),
        KL_METHOD_AUTO(UIContext, SetSizeMode, "Set size mode"),
        KL_METHOD_AUTO(UIContext, UpdateLayout, "Update layout"),
        KL_METHOD_AUTO(UIContext, ProcessEvent, "Process event"),
        KL_METHOD_AUTO(UIContext, HandlePointerUp, "Handle pointer up"),
        KL_METHOD_AUTO(UIContext, HandlePointerMove, "Handle pointer move"),
        KL_METHOD_AUTO(UIContext, HandleKeyDown, "Handle key down"),
        KL_METHOD_AUTO(UIContext, HandleTextInput, "Handle text input"),
        KL_METHOD_AUTO(UIContext, HandleScroll, "Handle scroll"),
        KL_METHOD_AUTO(UIContext, FocusedWidget, "Focused widget"),
        KL_METHOD_AUTO(UIContext, HoveredWidget, "Hovered widget"),
        KL_METHOD_AUTO(UIContext, ViewportWidth, "Viewport width"),
        KL_METHOD_AUTO(UIContext, ViewportHeight, "Viewport height")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIContext)
        KL_CTOR0(UIContext)
    KL_END_DESCRIBE(UIContext)

};

} // namespace ui
} // namespace koilo
