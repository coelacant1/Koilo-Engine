// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_context.hpp
 * @brief UIContext owns a widget tree, theme, event queue, and drives
 *        layout and event dispatch for one isolated UI context.
 *
 * Separate UIContext instances are used for editor and game UI.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include "widget.hpp"
#include "layout.hpp"
#include "event.hpp"
#include "style.hpp"
#include <string>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

class UIContext {
public:
    /// Callback type for script function invocation.
    using ScriptCallbackFn = std::function<void(const char* fnName)>;

    explicit UIContext(size_t poolCapacity = WidgetPool::DEFAULT_CAPACITY)
        : pool_(poolCapacity) {}

    /// Set the callback used to invoke script functions (e.g. OnClick handlers).
    void SetScriptCallback(ScriptCallbackFn fn) { scriptCallback_ = std::move(fn); }

    // ---- Widget Tree Management ----

    /// Create a widget and return its pool index. Returns -1 on failure.
    int CreateWidget(WidgetTag tag, const char* id) {
        int idx = pool_.Allocate();
        if (idx < 0) return -1;
        Widget* w = pool_.Get(idx);
        w->tag = tag;
        w->id = strings_.Intern(id);
        // Set focusable based on tag
        switch (tag) {
            case WidgetTag::Button:
            case WidgetTag::TextField:
            case WidgetTag::Slider:
            case WidgetTag::Checkbox:
            case WidgetTag::Dropdown:
            case WidgetTag::TreeNode:
            case WidgetTag::ToggleSwitch:
            case WidgetTag::RadioButton:
            case WidgetTag::NumberSpinner:
                w->flags.focusable = 1;
                break;
            default:
                break;
        }
        return idx;
    }

    /// Convenience: create typed widgets
    int CreatePanel(const char* id)  { return CreateWidget(WidgetTag::Panel, id); }
    int CreateLabel(const char* id, const char* text) {
        int idx = CreateWidget(WidgetTag::Label, id);
        if (idx >= 0) {
            pool_.Get(idx)->textId = strings_.Intern(text);
        }
        return idx;
    }
    int CreateButton(const char* id, const char* text) {
        int idx = CreateWidget(WidgetTag::Button, id);
        if (idx >= 0) {
            pool_.Get(idx)->textId = strings_.Intern(text);
        }
        return idx;
    }
    int CreateSlider(const char* id, float min, float max, float value) {
        int idx = CreateWidget(WidgetTag::Slider, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->sliderMin = min;
            w->sliderMax = max;
            w->sliderValue = value;
        }
        return idx;
    }
    int CreateCheckbox(const char* id, bool checked) {
        int idx = CreateWidget(WidgetTag::Checkbox, id);
        if (idx >= 0) {
            pool_.Get(idx)->checked = checked;
        }
        return idx;
    }
    int CreateTextField(const char* id, const char* placeholder = "") {
        int idx = CreateWidget(WidgetTag::TextField, id);
        if (idx >= 0) {
            pool_.Get(idx)->placeholderId = strings_.Intern(placeholder);
        }
        return idx;
    }
    int CreateSeparator(const char* id) {
        int idx = CreateWidget(WidgetTag::Separator, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->localH = 1.0f;
            w->widthMode = SizeMode::FillRemaining;
        }
        return idx;
    }
    int CreateProgressBar(const char* id, float value = 0.0f) {
        int idx = CreateWidget(WidgetTag::ProgressBar, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->progressValue = value;
            w->localH = 8.0f;
            w->widthMode = SizeMode::FillRemaining;
        }
        return idx;
    }
    int CreateToggleSwitch(const char* id, bool on = false) {
        int idx = CreateWidget(WidgetTag::ToggleSwitch, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->checked = on;
            w->localW = 36.0f;
            w->localH = 20.0f;
        }
        return idx;
    }
    int CreateRadioButton(const char* id, const char* group, bool selected = false) {
        int idx = CreateWidget(WidgetTag::RadioButton, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->radioGroup = strings_.Intern(group);
            w->checked = selected;
            w->localW = 18.0f;
            w->localH = 18.0f;
        }
        return idx;
    }
    int CreateNumberSpinner(const char* id, float value = 0.0f, float min = 0.0f,
                            float max = 100.0f, float step = 1.0f) {
        int idx = CreateWidget(WidgetTag::NumberSpinner, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->sliderValue = value;
            w->sliderMin = min;
            w->sliderMax = max;
            w->spinnerStep = step;
            w->localH = 22.0f;
            w->widthMode = SizeMode::FillRemaining;
        }
        return idx;
    }

    int CreateDockContainer(const char* id) {
        int idx = CreateWidget(WidgetTag::DockContainer, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->widthMode = SizeMode::FillRemaining;
            w->heightMode = SizeMode::FillRemaining;
            w->layout.direction = LayoutDirection::Row;
            w->flags.resizable = 1;
        }
        return idx;
    }

    int CreateSplitPane(const char* id, bool vertical = true) {
        int idx = CreateWidget(WidgetTag::SplitPane, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->widthMode = SizeMode::FillRemaining;
            w->heightMode = SizeMode::FillRemaining;
            w->layout.direction = vertical ? LayoutDirection::Column : LayoutDirection::Row;
            w->flags.resizable = 1;
        }
        return idx;
    }

    /// Create a floating panel overlay. Attach to root for screen-space positioning.
    int CreateFloatingPanel(const char* id, const char* title,
                            float x, float y, float w, float h) {
        int idx = CreateWidget(WidgetTag::FloatingPanel, id);
        if (idx < 0) return -1;
        Widget* widget = pool_.Get(idx);
        widget->panelTitleId = strings_.Intern(title);
        widget->positionMode = PositionMode::Absolute;
        widget->posLeft = x;
        widget->posTop = y;
        widget->localW = w;
        widget->localH = h;
        widget->widthMode = SizeMode::Fixed;
        widget->heightMode = SizeMode::Fixed;
        widget->zOrder = 500;
        widget->layout.direction = LayoutDirection::Column;
        widget->flags.clipChildren = 1;
        widget->minWidth = 120.0f;
        widget->minHeight = 80.0f;
        return idx;
    }

    /// Undock a panel from its parent and convert it to a floating panel.
    int UndockPanel(int panelIdx, float screenX, float screenY) {
        Widget* panel = pool_.Get(panelIdx);
        if (!panel) return -1;

        // Remember where it was docked
        int oldParent = panel->parent;
        int oldPos = -1;
        if (oldParent >= 0) {
            Widget* par = pool_.Get(oldParent);
            if (par) {
                for (int c = 0; c < par->childCount; ++c) {
                    if (par->children[c] == panelIdx) { oldPos = c; break; }
                }
            }
        }

        // Create a floating panel wrapper
        std::string floatId = std::string("float-") + strings_.Lookup(panel->id);
        const char* title = panel->panelTitleId != NullStringId
            ? strings_.Lookup(panel->panelTitleId)
            : strings_.Lookup(panel->id);

        float w = panel->computedRect.w > 0 ? panel->computedRect.w : panel->localW;
        float h = panel->computedRect.h > 0 ? panel->computedRect.h : panel->localH;
        int floatIdx = CreateFloatingPanel(floatId.c_str(), title,
                                           screenX, screenY, w, h + Widget::TITLE_BAR_HEIGHT);
        if (floatIdx < 0) return -1;

        Widget* floatPanel = pool_.Get(floatIdx);
        floatPanel->dockedParentIdx = oldParent;
        floatPanel->dockedChildPos = oldPos;

        // Remove panel from old parent and reparent under floating wrapper
        if (oldParent >= 0) {
            Widget* par = pool_.Get(oldParent);
            if (par) RemoveChild(*par, panelIdx);
            panel->parent = -1;
        }

        // Content area: panel fills the floating wrapper below the title bar
        panel->widthMode = SizeMode::FillRemaining;
        panel->heightMode = SizeMode::FillRemaining;
        panel->positionMode = PositionMode::Static;
        SetParent(panelIdx, floatIdx);

        // Attach floating panel to root
        SetParent(floatIdx, rootIndex_);

        // Assign highest z-order among floating panels
        BringToFront(floatIdx);

        MarkDirty(-1);
        return floatIdx;
    }

    /// Re-dock a floating panel back to its original location.
    bool RedockPanel(int floatIdx) {
        Widget* floatPanel = pool_.Get(floatIdx);
        if (!floatPanel || floatPanel->tag != WidgetTag::FloatingPanel) return false;

        int dockedParent = floatPanel->dockedParentIdx;
        int dockedPos = floatPanel->dockedChildPos;
        if (dockedParent < 0) return false;

        Widget* parent = pool_.Get(dockedParent);
        if (!parent) return false;

        // The content panel is the first child of the floating wrapper
        if (floatPanel->childCount == 0) return false;
        int contentIdx = floatPanel->children[0];
        Widget* content = pool_.Get(contentIdx);
        if (!content) return false;

        // Remove content from floating wrapper
        RemoveChild(*floatPanel, contentIdx);
        content->parent = -1;

        // Restore original sizing
        content->positionMode = PositionMode::Static;

        // Insert back into original parent at original position
        if (dockedPos >= 0 && dockedPos <= parent->childCount && parent->childCount < MAX_CHILDREN) {
            // Shift children to make room
            for (int c = parent->childCount; c > dockedPos; --c) {
                parent->children[c] = parent->children[c - 1];
            }
            parent->children[dockedPos] = static_cast<int16_t>(contentIdx);
            parent->childCount++;
            content->parent = static_cast<int16_t>(dockedParent);
        } else {
            SetParent(contentIdx, dockedParent);
        }

        // Destroy the floating wrapper
        DestroyWidget(floatIdx);
        MarkDirty(-1);
        return true;
    }

    /// Bring a floating panel to front (highest z-order).
    void BringToFront(int panelIdx) {
        Widget* panel = pool_.Get(panelIdx);
        if (!panel) return;
        int16_t maxZ = 500;
        // Find current max z-order among floating panels
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            const Widget* w = pool_.Get(static_cast<int>(i));
            if (w && w->tag == WidgetTag::FloatingPanel && static_cast<int>(i) != panelIdx) {
                if (w->zOrder >= maxZ) maxZ = w->zOrder + 1;
            }
        }
        panel->zOrder = maxZ;
    }

    int CreateTabBar(const char* id) {
        int idx = CreateWidget(WidgetTag::TabBar, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->widthMode = SizeMode::FillRemaining;
            w->localH = 28.0f;
            w->layout.direction = LayoutDirection::Row;
        }
        return idx;
    }

    int CreatePopupMenu(const char* id) {
        int idx = CreateWidget(WidgetTag::PopupMenu, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->flags.visible = false;
            w->layout.direction = LayoutDirection::Column;
        }
        return idx;
    }

    int CreateMenuItem(const char* id, const char* text) {
        int idx = CreateWidget(WidgetTag::MenuItem, id);
        if (idx >= 0) {
            Widget* w = pool_.Get(idx);
            w->textId = strings_.Intern(text);
            w->widthMode = SizeMode::FillRemaining;
            w->localH = 24.0f;
        }
        return idx;
    }

    // ---- Tree Node API ----

    /// Create a tree node and attach it to a parent container (e.g. scrollview).
    /// Automatically sets treeDepth based on sibling context.
    int AddTreeNode(int containerIdx, const char* id, const char* text,
                    int depth = 0, bool hasChildren = false) {
        int idx = CreateWidget(WidgetTag::TreeNode, id);
        if (idx < 0) return -1;
        Widget* w = pool_.Get(idx);
        w->textId = strings_.Intern(text);
        w->widthMode = SizeMode::FillRemaining;
        w->localH = 22.0f;
        w->layout.direction = LayoutDirection::Column;
        w->treeDepth = depth;
        w->treeHasChildren = hasChildren;
        if (containerIdx >= 0) SetParent(idx, containerIdx);
        return idx;
    }

    /// Remove a tree node and collapse its visual children (nodes at deeper depths
    /// that immediately follow it in the flat sibling list).
    void RemoveTreeNode(int nodeIdx) {
        Widget* w = pool_.Get(nodeIdx);
        if (!w || w->tag != WidgetTag::TreeNode) return;
        int depth = w->treeDepth;
        int parentIdx = w->parent;

        // Find and remove visual children (deeper siblings after this node)
        if (parentIdx >= 0) {
            Widget* par = pool_.Get(parentIdx);
            if (par) {
                int myPos = -1;
                for (int c = 0; c < par->childCount; ++c) {
                    if (par->children[c] == nodeIdx) { myPos = c; break; }
                }
                if (myPos >= 0) {
                    // Collect visual children to remove
                    std::vector<int> toRemove;
                    for (int c = myPos + 1; c < par->childCount; ++c) {
                        Widget* sib = pool_.Get(par->children[c]);
                        if (!sib || sib->tag != WidgetTag::TreeNode) break;
                        if (sib->treeDepth <= depth) break;
                        toRemove.push_back(par->children[c]);
                    }
                    for (int ri : toRemove) {
                        DeselectNode(ri);
                        DestroyWidget(ri);
                    }
                }
            }
        }
        DeselectNode(nodeIdx);
        DestroyWidget(nodeIdx);
        RefreshTreeFlags(parentIdx);
    }

    /// Reparent a tree node: move it (and its visual children) to a new position.
    /// insertAfterIdx = -1 means insert at the end of newParent's children at targetDepth.
    void ReparentTreeNode(int nodeIdx, int containerIdx, int insertAfterIdx, int newDepth) {
        Widget* w = pool_.Get(nodeIdx);
        if (!w || w->tag != WidgetTag::TreeNode) return;

        int oldDepth = w->treeDepth;
        int depthDelta = newDepth - oldDepth;

        // Collect this node + its visual children
        std::vector<int> moving;
        moving.push_back(nodeIdx);

        int oldParent = w->parent;
        if (oldParent >= 0) {
            Widget* par = pool_.Get(oldParent);
            if (par) {
                int myPos = -1;
                for (int c = 0; c < par->childCount; ++c) {
                    if (par->children[c] == nodeIdx) { myPos = c; break; }
                }
                if (myPos >= 0) {
                    for (int c = myPos + 1; c < par->childCount; ++c) {
                        Widget* sib = pool_.Get(par->children[c]);
                        if (!sib || sib->tag != WidgetTag::TreeNode) break;
                        if (sib->treeDepth <= oldDepth) break;
                        moving.push_back(par->children[c]);
                    }
                }
            }
        }

        // Remove from old parent
        for (int idx : moving) {
            Widget* node = pool_.Get(idx);
            if (node && node->parent >= 0) {
                Widget* p = pool_.Get(node->parent);
                if (p) RemoveChild(*p, idx);
                node->parent = -1;
            }
        }

        // Adjust depths
        for (int idx : moving) {
            Widget* node = pool_.Get(idx);
            if (node) node->treeDepth += depthDelta;
        }

        // Insert into new container
        Widget* container = pool_.Get(containerIdx);
        if (!container) return;

        int insertPos = container->childCount;
        if (insertAfterIdx >= 0) {
            for (int c = 0; c < container->childCount; ++c) {
                if (container->children[c] == insertAfterIdx) {
                    // Find end of insertAfter's visual subtree
                    Widget* afterW = pool_.Get(insertAfterIdx);
                    int afterDepth = afterW ? afterW->treeDepth : 0;
                    insertPos = c + 1;
                    for (int s = c + 1; s < container->childCount; ++s) {
                        Widget* sub = pool_.Get(container->children[s]);
                        if (!sub || sub->tag != WidgetTag::TreeNode) break;
                        if (sub->treeDepth <= afterDepth) break;
                        insertPos = s + 1;
                    }
                    break;
                }
            }
        }

        // Shift existing children to make room
        int moveCount = static_cast<int>(moving.size());
        if (container->childCount + moveCount > MAX_CHILDREN) return;
        for (int c = container->childCount - 1; c >= insertPos; --c) {
            container->children[c + moveCount] = container->children[c];
        }
        for (int i = 0; i < moveCount; ++i) {
            container->children[insertPos + i] = static_cast<int16_t>(moving[i]);
            Widget* node = pool_.Get(moving[i]);
            if (node) node->parent = static_cast<int16_t>(containerIdx);
        }
        container->childCount += moveCount;

        RefreshTreeFlags(containerIdx);
        MarkDirty(-1);
    }

    /// Recalculate treeHasChildren for all tree nodes in a container based on
    /// the flat sibling order and depth values.
    void RefreshTreeFlags(int containerIdx) {
        Widget* container = pool_.Get(containerIdx);
        if (!container) return;
        for (int c = 0; c < container->childCount; ++c) {
            Widget* w = pool_.Get(container->children[c]);
            if (!w || w->tag != WidgetTag::TreeNode) continue;
            // Check if next sibling is deeper (visual child)
            bool hasKids = false;
            if (c + 1 < container->childCount) {
                Widget* next = pool_.Get(container->children[c + 1]);
                if (next && next->tag == WidgetTag::TreeNode && next->treeDepth > w->treeDepth) {
                    hasKids = true;
                }
            }
            w->treeHasChildren = hasKids;
        }
        MarkDirty(containerIdx);
    }

    // ---- Selection API ----

    /// Select a single tree node, clearing any previous selection.
    void SelectNode(int nodeIdx) {
        ClearSelection();
        Widget* w = pool_.Get(nodeIdx);
        if (!w) return;
        w->flags.selected = 1;
        selectedNodes_.push_back(nodeIdx);
        theme_.NotifyStateChange(*w, nodeIdx);
    }

    /// Toggle selection on a node (for Ctrl+Click).
    void ToggleSelectNode(int nodeIdx) {
        Widget* w = pool_.Get(nodeIdx);
        if (!w) return;
        if (w->flags.selected) {
            DeselectNode(nodeIdx);
        } else {
            w->flags.selected = 1;
            selectedNodes_.push_back(nodeIdx);
            theme_.NotifyStateChange(*w, nodeIdx);
        }
    }

    /// Range-select tree nodes between anchor and target (for Shift+Click).
    void RangeSelectNodes(int anchorIdx, int targetIdx) {
        // Both must share the same parent container
        Widget* anchor = pool_.Get(anchorIdx);
        Widget* target = pool_.Get(targetIdx);
        if (!anchor || !target) return;
        if (anchor->parent != target->parent) return;

        Widget* par = pool_.Get(anchor->parent);
        if (!par) return;

        int posA = -1, posB = -1;
        for (int c = 0; c < par->childCount; ++c) {
            if (par->children[c] == anchorIdx) posA = c;
            if (par->children[c] == targetIdx) posB = c;
        }
        if (posA < 0 || posB < 0) return;
        if (posA > posB) std::swap(posA, posB);

        ClearSelection();
        for (int c = posA; c <= posB; ++c) {
            Widget* w = pool_.Get(par->children[c]);
            if (w && w->tag == WidgetTag::TreeNode && w->flags.visible) {
                w->flags.selected = 1;
                selectedNodes_.push_back(par->children[c]);
                theme_.NotifyStateChange(*w, par->children[c]);
            }
        }
    }

    /// Deselect a specific node.
    void DeselectNode(int nodeIdx) {
        Widget* w = pool_.Get(nodeIdx);
        if (!w) return;
        w->flags.selected = 0;
        selectedNodes_.erase(
            std::remove(selectedNodes_.begin(), selectedNodes_.end(), nodeIdx),
            selectedNodes_.end());
        theme_.NotifyStateChange(*w, nodeIdx);
    }

    /// Clear all selection.
    void ClearSelection() {
        for (int idx : selectedNodes_) {
            Widget* w = pool_.Get(idx);
            if (w) {
                w->flags.selected = 0;
                theme_.NotifyStateChange(*w, idx);
            }
        }
        selectedNodes_.clear();
    }

    /// Get currently selected node indices.
    const std::vector<int>& SelectedNodes() const { return selectedNodes_; }

    /// Selection anchor for Shift+Click range selection.
    int SelectionAnchor() const { return selectionAnchor_; }

    /// Attach child to parent. Returns true on success.
    bool SetParent(int childIdx, int parentIdx) {
        Widget* child = pool_.Get(childIdx);
        Widget* parent = pool_.Get(parentIdx);
        if (!child || !parent) return false;
        if (parent->childCount >= MAX_CHILDREN) return false;

        // Remove from old parent
        if (child->parent >= 0) {
            Widget* oldParent = pool_.Get(child->parent);
            if (oldParent) RemoveChild(*oldParent, childIdx);
        }

        parent->children[parent->childCount++] = static_cast<int16_t>(childIdx);
        child->parent = static_cast<int16_t>(parentIdx);
        MarkDirty(parentIdx);
        return true;
    }

    /// Destroy a widget and all its children recursively.
    void DestroyWidget(int idx) {
        Widget* w = pool_.Get(idx);
        if (!w) return;

        // Destroy children first (iterate backwards to avoid index shift issues)
        while (w->childCount > 0) {
            DestroyWidget(w->children[w->childCount - 1]);
        }

        // Remove from parent
        if (w->parent >= 0) {
            Widget* p = pool_.Get(w->parent);
            if (p) RemoveChild(*p, idx);
        }

        // Clear focus if needed
        if (focusedWidget_ == idx) focusedWidget_ = -1;

        pool_.Free(idx);
    }

    /// Set the root widget index. This widget gets the screen rect.
    void SetRoot(int idx) { rootIndex_ = idx; }
    int Root() const { return rootIndex_; }

    // ---- Property Access ----

    Widget* GetWidget(int idx) { return pool_.Get(idx); }
    const Widget* GetWidget(int idx) const { return pool_.Get(idx); }

    void SetText(int idx, const char* text) {
        Widget* w = pool_.Get(idx);
        if (w) w->textId = strings_.Intern(text);
    }

    const char* GetText(int idx) const {
        const Widget* w = pool_.Get(idx);
        if (!w) return "";
        return strings_.Lookup(w->textId);
    }

    void SetVisible(int idx, bool v) {
        Widget* w = pool_.Get(idx);
        if (w) { w->flags.visible = v ? 1 : 0; MarkDirty(idx); }
    }

    void SetEnabled(int idx, bool e) {
        Widget* w = pool_.Get(idx);
        if (w) w->flags.enabled = e ? 1 : 0;
    }

    void SetSize(int idx, float width, float height) {
        Widget* w = pool_.Get(idx);
        if (w) {
            w->localW = width; w->localH = height;
            w->widthMode = SizeMode::Fixed;
            w->heightMode = SizeMode::Fixed;
            MarkDirty(idx);
        }
    }

    void SetPosition(int idx, float x, float y) {
        Widget* w = pool_.Get(idx);
        if (w) { w->localX = x; w->localY = y; MarkDirty(idx); }
    }

    void SetLayout(int idx, LayoutDirection dir, Alignment mainAlign = Alignment::Start,
                   Alignment crossAlign = Alignment::Start, float spacing = 0.0f) {
        Widget* w = pool_.Get(idx);
        if (!w) return;
        w->layout.direction = dir;
        w->layout.mainAlign = mainAlign;
        w->layout.crossAlign = crossAlign;
        w->layout.spacing = spacing;
        MarkDirty(idx);
    }

    void SetPadding(int idx, float top, float right, float bottom, float left) {
        Widget* w = pool_.Get(idx);
        if (!w) return;
        w->padding = {top, right, bottom, left};
        MarkDirty(idx);
    }

    void SetMargin(int idx, float top, float right, float bottom, float left) {
        Widget* w = pool_.Get(idx);
        if (!w) return;
        w->margin = {top, right, bottom, left};
        MarkDirty(idx);
    }

    void SetAnchors(int idx, float minX, float minY, float maxX, float maxY) {
        Widget* w = pool_.Get(idx);
        if (!w) return;
        w->anchors = {minX, minY, maxX, maxY};
        MarkDirty(idx);
    }

    void SetSizeMode(int idx, SizeMode wMode, SizeMode hMode) {
        Widget* w = pool_.Get(idx);
        if (!w) return;
        w->widthMode = wMode;
        w->heightMode = hMode;
        MarkDirty(idx);
    }

    // ---- Event Callbacks ----

    void SetOnClick(int idx, std::function<void(Widget&)> fn) {
        Widget* w = pool_.Get(idx);
        if (w) w->onClickCpp = std::move(fn);
    }

    void SetOnChange(int idx, std::function<void(Widget&)> fn) {
        Widget* w = pool_.Get(idx);
        if (w) w->onChangeCpp = std::move(fn);
    }

    void SetOnClickScript(int idx, const char* fnName) {
        Widget* w = pool_.Get(idx);
        if (w) w->onClickId = strings_.Intern(fnName);
    }

    void SetOnChangeScript(int idx, const char* fnName) {
        Widget* w = pool_.Get(idx);
        if (w) w->onChangeId = strings_.Intern(fnName);
    }

    // ---- Update Cycle ----

    /// Set the viewport size (typically the window size).
    void SetViewport(float width, float height) {
        if (viewportW_ != width || viewportH_ != height) {
            viewportW_ = width;
            viewportH_ = height;
            if (rootIndex_ >= 0) MarkDirty(rootIndex_);
        }
    }

    /// Run layout pass. Updates computedRect on all dirty widgets.
    void UpdateLayout() {
        if (rootIndex_ < 0) return;
        Widget* root = pool_.Get(rootIndex_);
        if (!root) return;

        // Root fills viewport
        root->computedRect = {0.0f, 0.0f, viewportW_, viewportH_};

        layout_.Compute(pool_, rootIndex_);
    }

    /// Process a raw input event. Performs hit testing, focus, and dispatch.
    void ProcessEvent(Event event) {
        if (rootIndex_ < 0) return;

        switch (event.type) {
            case EventType::PointerDown:
                if (event.pointerButton == 1) {
                    HandleRightClick(event);
                } else {
                    // Dismiss active popup on any left-click
                    DismissPopup();
                    HandlePointerDown(event);
                }
                break;
            case EventType::PointerUp:
                if (event.pointerButton == 0)
                    HandlePointerUp(event);
                break;
            case EventType::PointerMove:
                HandlePointerMove(event);
                break;
            case EventType::KeyDown:
                if (activePopup_ >= 0 && event.key == KeyCode::Escape) {
                    DismissPopup();
                } else {
                    HandleKeyDown(event);
                }
                break;
            case EventType::TextInput:
                HandleTextInput(event);
                break;
            case EventType::Scroll:
                HandleScroll(event);
                break;
            default:
                break;
        }
    }

    // ---- Query ----

    int FocusedWidget() const { return focusedWidget_; }
    int HoveredWidget() const { return hoveredWidget_; }
    float ViewportWidth() const { return viewportW_; }
    float ViewportHeight() const { return viewportH_; }

    const WidgetPool& Pool() const { return pool_; }
    WidgetPool& Pool() { return pool_; }
    const StringTable& Strings() const { return strings_; }
    StringTable& Strings() { return strings_; }
    const Theme& GetTheme() const { return theme_; }
    Theme& GetTheme() { return theme_; }

    /// Update transition animations. Call once per frame with delta time in seconds.
    void UpdateTransitions(float dt) {
        theme_.UpdateTransitions(dt);

        // Update tooltip timer
        if (hoveredWidget_ >= 0) {
            Widget* hw = pool_.Get(hoveredWidget_);
            if (hw && hw->tooltipId != NullStringId) {
                tooltipHoverTime_ += dt;
                if (tooltipHoverTime_ >= TOOLTIP_DELAY) {
                    tooltipWidget_ = hoveredWidget_;
                }
            } else {
                tooltipHoverTime_ = 0.0f;
                tooltipWidget_ = -1;
            }
        } else {
            tooltipHoverTime_ = 0.0f;
            tooltipWidget_ = -1;
        }

        // Update smooth scrolling
        UpdateSmoothScroll(dt);
    }

    /// Get active tooltip info. Returns widget index or -1 if no tooltip visible.
    int GetTooltipWidget() const { return tooltipWidget_; }
    float GetTooltipX() const { return tooltipX_; }
    float GetTooltipY() const { return tooltipY_; }

    /// Get the cursor style for the currently hovered widget.
    /// Checks active drags, floating panel edges, then widget chain.
    CursorType GetRequestedCursor() const {
        // Active panel drag - show appropriate resize cursor
        if (panelDrag_.active) {
            switch (panelDrag_.mode) {
            case PanelDragMode::Move:         return CursorType::Move;
            case PanelDragMode::ResizeLeft:
            case PanelDragMode::ResizeRight:  return CursorType::EWResize;
            case PanelDragMode::ResizeTop:
            case PanelDragMode::ResizeBottom: return CursorType::NSResize;
            case PanelDragMode::ResizeTL:
            case PanelDragMode::ResizeBR:     return CursorType::NWSEResize;
            case PanelDragMode::ResizeTR:
            case PanelDragMode::ResizeBL:     return CursorType::NESWResize;
            default: break;
            }
        }
        if (resizeDrag_.active) return CursorType::EWResize;

        // Check if mouse is over a floating panel edge (hover cursor)
        CursorType panelCur = GetFloatingPanelEdgeCursor(lastPointerX_, lastPointerY_);
        if (panelCur != CursorType::Default) return panelCur;

        int idx = hoveredWidget_;
        while (idx >= 0) {
            const Widget* w = pool_.Get(idx);
            if (!w) break;
            if (w->cursor != CursorType::Default) return w->cursor;
            idx = w->parent;
        }
        return CursorType::Default;
    }

    /// Find a widget by interned string ID (linear scan).
    int FindWidget(const char* id) const {
        StringId sid = const_cast<StringTable&>(strings_).Intern(id);
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            const Widget* w = pool_.Get(static_cast<int>(i));
            if (w && w->id == sid) return static_cast<int>(i);
        }
        return -1;
    }

private:
    static constexpr int MAX_SMOOTH_SCROLLS = 8;
    struct SmoothScroll {
        int widgetIdx = -1;
        float targetY = 0.0f;
        bool active = false;
    };

    void MarkDirty(int idx) {
        Widget* w = pool_.Get(idx);
        if (w) w->flags.dirty = 1;
    }

    SmoothScroll* FindOrCreateSmoothScroll(int widgetIdx) {
        // Find existing
        for (auto& ss : smoothScrolls_)
            if (ss.active && ss.widgetIdx == widgetIdx) return &ss;
        // Find free slot
        for (auto& ss : smoothScrolls_) {
            if (!ss.active) {
                ss.widgetIdx = widgetIdx;
                return &ss;
            }
        }
        return nullptr;
    }

    void UpdateSmoothScroll(float dt) {
        constexpr float SMOOTH_SPEED = 12.0f; // higher = faster interpolation
        for (auto& ss : smoothScrolls_) {
            if (!ss.active) continue;
            Widget* w = pool_.Get(ss.widgetIdx);
            if (!w) { ss.active = false; continue; }

            float diff = ss.targetY - w->scrollY;
            if (std::abs(diff) < 0.5f) {
                w->scrollY = ss.targetY;
                ss.active = false;
            } else {
                w->scrollY += diff * std::min(1.0f, SMOOTH_SPEED * dt);
            }
            MarkDirty(ss.widgetIdx);
        }
    }

    void RemoveChild(Widget& parent, int childIdx) {
        for (int i = 0; i < parent.childCount; ++i) {
            if (parent.children[i] == childIdx) {
                // Shift remaining children
                for (int j = i; j < parent.childCount - 1; ++j) {
                    parent.children[j] = parent.children[j + 1];
                }
                parent.children[--parent.childCount] = -1;
                return;
            }
        }
    }

    void SetFocus(int idx, bool fromKeyboard = false) {
        if (focusedWidget_ == idx) return;
        Widget* old = pool_.Get(focusedWidget_);
        if (old) { old->flags.focused = 0; old->flags.focusVisible = 0; theme_.NotifyStateChange(*old, focusedWidget_); }
        focusedWidget_ = idx;
        Widget* nw = pool_.Get(idx);
        if (nw && nw->flags.focusable) {
            nw->flags.focused = 1;
            nw->flags.focusVisible = fromKeyboard ? 1 : 0;
            theme_.NotifyStateChange(*nw, idx);
        }
    }

    void HandlePointerDown(Event& event) {
        lastPointerX_ = event.pointerX;
        lastPointerY_ = event.pointerY;
        lastModifiers_ = event.mods;

        // Check floating panel title bar / resize edge first
        if (FindFloatingPanelHit(event.pointerX, event.pointerY)) {
            return;
        }

        // Check minimized panel tray click
        if (FindMinimizedTrayHit(event.pointerX, event.pointerY)) {
            return;
        }

        // Check for divider hit (resize drag takes priority over widget clicks)
        if (FindDividerHit(event.pointerX, event.pointerY)) {
            resizeDrag_.active = true;
            return;
        }

        // Check for scrollbar thumb drag
        if (FindScrollbarHit(event.pointerX, event.pointerY)) {
            return;
        }

        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);
        if (hit >= 0) {
            Widget* w = pool_.Get(hit);
            if (w) {
                w->flags.pressed = 1;
                theme_.NotifyStateChange(*w, hit);
                if (w->flags.focusable) SetFocus(hit);
            }
        } else {
            SetFocus(-1);
        }
    }

    void HandlePointerUp(Event& event) {
        // End floating panel drag
        if (panelDrag_.active) {
            panelDrag_.active = false;
            return;
        }
        // End resize drag
        if (resizeDrag_.active) {
            resizeDrag_.active = false;
            return;
        }
        // End scroll drag
        if (scrollDrag_.active) {
            scrollDrag_.active = false;
            return;
        }

        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);

        // Release all pressed widgets
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            Widget* w = pool_.Get(static_cast<int>(i));
            if (w && w->flags.pressed) {
                w->flags.pressed = 0;
                theme_.NotifyStateChange(*w, static_cast<int>(i));
                // Click: pressed released over the same widget
                if (static_cast<int>(i) == hit) {
                    DispatchClick(*w);
                }
            }
        }
    }

    void HandlePointerMove(Event& event) {
        // Handle active floating panel drag
        if (panelDrag_.active) {
            HandlePanelDrag(event.pointerX, event.pointerY);
            return;
        }
        // Handle active resize drag
        if (resizeDrag_.active) {
            HandleResizeDrag(event.pointerX, event.pointerY);
            return;
        }
        // Handle scrollbar drag
        if (scrollDrag_.active) {
            HandleScrollDrag(event.pointerY);
            return;
        }

        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);

        // Update hovered state
        if (hit != hoveredWidget_) {
            Widget* old = pool_.Get(hoveredWidget_);
            if (old) { old->flags.hovered = 0; theme_.NotifyStateChange(*old, hoveredWidget_); }
            hoveredWidget_ = hit;
            Widget* nw = pool_.Get(hit);
            if (nw) { nw->flags.hovered = 1; theme_.NotifyStateChange(*nw, hit); }
            // Reset tooltip timer on hover change
            tooltipHoverTime_ = 0.0f;
            tooltipWidget_ = -1;
        }
        // Track mouse position for tooltip
        tooltipX_ = event.pointerX;
        tooltipY_ = event.pointerY;

        // Slider drag
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            Widget* w = pool_.Get(static_cast<int>(i));
            if (w && w->flags.pressed && w->tag == WidgetTag::Slider) {
                float relX = event.pointerX - w->computedRect.x;
                float t = relX / w->computedRect.w;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                w->sliderValue = w->sliderMin + t * (w->sliderMax - w->sliderMin);
                if (w->onChangeCpp) w->onChangeCpp(*w);
            }
        }
    }

    void HandleKeyDown(Event& event) {
        if (event.key == KeyCode::Tab) {
            // Tab navigation
            focus_.RebuildOrder(pool_, rootIndex_);
            focus_.SetCurrent(focusedWidget_);
            int next = event.mods.shift ? focus_.Prev() : focus_.Next();
            if (next >= 0) SetFocus(next, true); // keyboard focus -> focusVisible
            return;
        }

        Widget* focused = pool_.Get(focusedWidget_);
        if (!focused) return;

        // Checkbox toggle
        if (focused->tag == WidgetTag::Checkbox &&
            (event.key == KeyCode::Space || event.key == KeyCode::Return)) {
            focused->checked = !focused->checked;
            if (focused->onChangeCpp) focused->onChangeCpp(*focused);
        }

        // ToggleSwitch toggle
        if (focused->tag == WidgetTag::ToggleSwitch &&
            (event.key == KeyCode::Space || event.key == KeyCode::Return)) {
            focused->checked = !focused->checked;
            if (focused->onChangeCpp) focused->onChangeCpp(*focused);
        }

        // RadioButton select
        if (focused->tag == WidgetTag::RadioButton &&
            (event.key == KeyCode::Space || event.key == KeyCode::Return)) {
            if (!focused->checked) {
                focused->checked = true;
                if (focused->parent >= 0) {
                    Widget* p = pool_.Get(focused->parent);
                    if (p) {
                        for (int ci = 0; ci < p->childCount; ++ci) {
                            Widget* sib = pool_.Get(p->children[ci]);
                            if (sib && sib != focused &&
                                sib->tag == WidgetTag::RadioButton &&
                                sib->radioGroup == focused->radioGroup) {
                                sib->checked = false;
                            }
                        }
                    }
                }
                if (focused->onChangeCpp) focused->onChangeCpp(*focused);
            }
        }

        // NumberSpinner adjust with arrow keys
        if (focused->tag == WidgetTag::NumberSpinner) {
            if (event.key == KeyCode::Up || event.key == KeyCode::Right) {
                focused->sliderValue = std::min(focused->sliderMax,
                    focused->sliderValue + focused->spinnerStep);
                if (focused->onChangeCpp) focused->onChangeCpp(*focused);
            } else if (event.key == KeyCode::Down || event.key == KeyCode::Left) {
                focused->sliderValue = std::max(focused->sliderMin,
                    focused->sliderValue - focused->spinnerStep);
                if (focused->onChangeCpp) focused->onChangeCpp(*focused);
            }
        }

        // Dropdown toggle
        if (focused->tag == WidgetTag::Dropdown && event.key == KeyCode::Return) {
            focused->dropdownOpen = !focused->dropdownOpen;
        }

        // TreeNode toggle
        if (focused->tag == WidgetTag::TreeNode && event.key == KeyCode::Return) {
            focused->expanded = !focused->expanded;
            MarkDirty(focusedWidget_);
        }

        // TreeNode keyboard navigation
        if (focused->tag == WidgetTag::TreeNode) {
            if (event.key == KeyCode::Up || event.key == KeyCode::Down ||
                event.key == KeyCode::Left || event.key == KeyCode::Right ||
                event.key == KeyCode::Home || event.key == KeyCode::End) {
                HandleTreeNavigation(event.key, event.mods);
            }
        }
    }

    void HandleTextInput(Event& event) {
        Widget* focused = pool_.Get(focusedWidget_);
        if (!focused || focused->tag != WidgetTag::TextField) return;
        // Text input handling delegated to text field widget
        // (will be expanded when text editing is fully implemented)
    }

    void HandleScroll(Event& event) {
        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);

        // NumberSpinner: scroll adjusts value
        if (hit >= 0) {
            Widget* w = pool_.Get(hit);
            if (w && w->tag == WidgetTag::NumberSpinner) {
                float dir = event.scrollDelta > 0 ? 1.0f : -1.0f;
                w->sliderValue = std::max(w->sliderMin,
                    std::min(w->sliderMax, w->sliderValue + w->spinnerStep * dir));
                if (w->onChangeCpp) w->onChangeCpp(*w);
                return;
            }
        }

        // Walk up to find nearest scrollable element (ScrollView or overflow: scroll)
        while (hit >= 0) {
            Widget* w = pool_.Get(hit);
            if (!w) break;
            if (w->tag == WidgetTag::ScrollView || w->flags.scrollable) {
                float delta = event.scrollDelta * 20.0f;
                if (w->scrollSmooth) {
                    // Smooth scroll: update target, interpolation in UpdateSmoothScroll
                    SmoothScroll* ss = FindOrCreateSmoothScroll(hit);
                    if (ss) {
                        if (!ss->active) {
                            ss->targetY = w->scrollY;
                            ss->active = true;
                        }
                        ss->targetY += delta;
                    }
                } else {
                    w->scrollY += delta;
                }
                MarkDirty(hit);
                return;
            }
            hit = w->parent;
        }
    }

    void HandleRightClick(Event& event) {
        // Dismiss any existing popup first
        DismissPopup();

        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);
        if (hit < 0) return;

        // Walk up from hit target to find a sibling PopupMenu
        int idx = hit;
        while (idx >= 0) {
            Widget* w = pool_.Get(idx);
            if (!w) break;
            int parentIdx = w->parent;
            Widget* p = pool_.Get(parentIdx);
            if (p) {
                for (int i = 0; i < p->childCount; ++i) {
                    Widget* sibling = pool_.Get(p->children[i]);
                    if (sibling && sibling->tag == WidgetTag::PopupMenu) {
                        ShowPopup(p->children[i], event.pointerX, event.pointerY);
                        return;
                    }
                }
            }
            idx = parentIdx;
        }
    }

    void ShowPopup(int popupIdx, float x, float y) {
        Widget* popup = pool_.Get(popupIdx);
        if (!popup) return;
        popup->flags.visible = true;
        popup->positionMode = PositionMode::Absolute;
        popup->posLeft = x;
        popup->posTop = y;
        popup->zOrder = 1000; // render on top
        activePopup_ = popupIdx;
        MarkDirty(popupIdx);
    }

    void DismissPopup() {
        if (activePopup_ >= 0) {
            Widget* popup = pool_.Get(activePopup_);
            if (popup) {
                popup->flags.visible = false;
                popup->zOrder = 0;
                MarkDirty(activePopup_);
            }
            activePopup_ = -1;
        }
    }

    /// Collect visible tree node indices in display order from a container.
    std::vector<int> GetVisibleTreeNodes(int containerIdx) const {
        std::vector<int> result;
        const Widget* container = pool_.Get(containerIdx);
        if (!container) return result;

        // Track which depths are currently collapsed
        bool collapsed[32] = {};
        for (int c = 0; c < container->childCount; ++c) {
            const Widget* w = pool_.Get(container->children[c]);
            if (!w || w->tag != WidgetTag::TreeNode) continue;
            if (!w->flags.visible) continue;

            int d = w->treeDepth;
            // Check if any ancestor depth is collapsed
            bool hidden = false;
            for (int ad = 0; ad < d && ad < 32; ++ad) {
                if (collapsed[ad]) { hidden = true; break; }
            }
            if (hidden) continue;

            result.push_back(container->children[c]);

            // If this node has children but is collapsed, mark its depth
            if (d < 32) {
                collapsed[d] = (w->treeHasChildren && !w->expanded);
                // Clear deeper collapsed states since we're at a new branch
                for (int cd = d + 1; cd < 32; ++cd) collapsed[cd] = false;
            }
        }
        return result;
    }

    /// Scroll a scrollview so that a child widget is visible.
    void ScrollNodeIntoView(int nodeIdx) {
        Widget* w = pool_.Get(nodeIdx);
        if (!w) return;
        int parentIdx = w->parent;
        Widget* parent = pool_.Get(parentIdx);
        if (!parent) return;
        // Walk up to find the scrollview ancestor
        while (parent && parent->tag != WidgetTag::ScrollView && !parent->flags.scrollable) {
            parentIdx = parent->parent;
            parent = pool_.Get(parentIdx);
        }
        if (!parent) return;

        Rect pr = parent->computedRect;
        Rect wr = w->computedRect;

        // Check if the widget is above or below the visible area
        float visTop = pr.y;
        float visBot = pr.y + pr.h;
        float nodeTop = wr.y;
        float nodeBot = wr.y + wr.h;

        if (nodeTop < visTop) {
            parent->scrollY -= (visTop - nodeTop);
        } else if (nodeBot > visBot) {
            parent->scrollY += (nodeBot - visBot);
        }
        if (parent->scrollY < 0.0f) parent->scrollY = 0.0f;
        MarkDirty(parentIdx);
    }

    /// Handle arrow key navigation within a tree node structure.
    void HandleTreeNavigation(KeyCode key, Modifiers mods) {
        Widget* focused = pool_.Get(focusedWidget_);
        if (!focused || focused->tag != WidgetTag::TreeNode) return;

        int containerIdx = focused->parent;
        if (containerIdx < 0) return;

        if (key == KeyCode::Left) {
            if (focused->treeHasChildren && focused->expanded) {
                focused->expanded = false;
                MarkDirty(-1);
            } else if (focused->treeDepth > 0) {
                // Jump to visual parent: scan backward for node at depth-1
                Widget* container = pool_.Get(containerIdx);
                if (!container) return;
                int myPos = -1;
                for (int c = 0; c < container->childCount; ++c) {
                    if (container->children[c] == focusedWidget_) { myPos = c; break; }
                }
                for (int c = myPos - 1; c >= 0; --c) {
                    Widget* w = pool_.Get(container->children[c]);
                    if (w && w->tag == WidgetTag::TreeNode && w->treeDepth == focused->treeDepth - 1) {
                        SetFocus(container->children[c], true);
                        SelectNode(container->children[c]);
                        selectionAnchor_ = container->children[c];
                        ScrollNodeIntoView(container->children[c]);
                        return;
                    }
                }
            }
            return;
        }

        if (key == KeyCode::Right) {
            if (focused->treeHasChildren && !focused->expanded) {
                focused->expanded = true;
                MarkDirty(-1);
            } else if (focused->treeHasChildren && focused->expanded) {
                // Move to first child
                auto visible = GetVisibleTreeNodes(containerIdx);
                for (size_t i = 0; i < visible.size(); ++i) {
                    if (visible[i] == focusedWidget_ && i + 1 < visible.size()) {
                        SetFocus(visible[i + 1], true);
                        SelectNode(visible[i + 1]);
                        selectionAnchor_ = visible[i + 1];
                        ScrollNodeIntoView(visible[i + 1]);
                        return;
                    }
                }
            }
            return;
        }

        // Up / Down / Home / End: navigate among visible nodes
        auto visible = GetVisibleTreeNodes(containerIdx);
        if (visible.empty()) return;

        int curPos = -1;
        for (size_t i = 0; i < visible.size(); ++i) {
            if (visible[i] == focusedWidget_) { curPos = static_cast<int>(i); break; }
        }

        int targetPos = curPos;
        if (key == KeyCode::Up && curPos > 0) {
            targetPos = curPos - 1;
        } else if (key == KeyCode::Down && curPos >= 0 && curPos + 1 < static_cast<int>(visible.size())) {
            targetPos = curPos + 1;
        } else if (key == KeyCode::Home) {
            targetPos = 0;
        } else if (key == KeyCode::End) {
            targetPos = static_cast<int>(visible.size()) - 1;
        }

        if (targetPos != curPos && targetPos >= 0) {
            int targetIdx = visible[targetPos];
            SetFocus(targetIdx, true);
            if (mods.shift && selectionAnchor_ >= 0) {
                RangeSelectNodes(selectionAnchor_, targetIdx);
            } else if (!mods.shift) {
                SelectNode(targetIdx);
                selectionAnchor_ = targetIdx;
            }
            ScrollNodeIntoView(targetIdx);
        }
    }

    void DispatchClick(Widget& w) {
        // First, handle tag-specific behavior on the direct target
        bool handled = false;
        switch (w.tag) {
            case WidgetTag::Checkbox:
                w.checked = !w.checked;
                if (w.onChangeCpp) w.onChangeCpp(w);
                if (w.onChangeId != NullStringId && scriptCallback_) {
                    const char* fn = strings_.Lookup(w.onChangeId);
                    if (fn && fn[0] != '\0') scriptCallback_(fn);
                }
                handled = true;
                break;
            case WidgetTag::Dropdown:
                w.dropdownOpen = !w.dropdownOpen;
                handled = true;
                break;
            case WidgetTag::TreeNode: {
                // Determine if click was on the expand box or the text area
                float indent = w.treeDepth * 16.0f;
                float boxSize = 10.0f;
                Rect cr = w.computedRect;
                float boxLeft = cr.x + w.padding.left + indent;
                float boxRight = boxLeft + boxSize;

                bool clickedBox = w.treeHasChildren &&
                    lastPointerX_ >= boxLeft && lastPointerX_ <= boxRight;

                if (clickedBox) {
                    w.expanded = !w.expanded;
                } else {
                    // Selection logic
                    int widgetIdx = -1;
                    for (size_t si = 0; si < pool_.Capacity(); ++si) {
                        if (pool_.Get(static_cast<int>(si)) == &w) {
                            widgetIdx = static_cast<int>(si);
                            break;
                        }
                    }
                    if (widgetIdx >= 0) {
                        if (lastModifiers_.ctrl) {
                            ToggleSelectNode(widgetIdx);
                        } else if (lastModifiers_.shift && selectionAnchor_ >= 0) {
                            RangeSelectNodes(selectionAnchor_, widgetIdx);
                        } else {
                            SelectNode(widgetIdx);
                            selectionAnchor_ = widgetIdx;
                        }
                    }
                }
                MarkDirty(-1);
                handled = true;
                break;
            }
            case WidgetTag::ToggleSwitch:
                w.checked = !w.checked;
                if (w.onChangeCpp) w.onChangeCpp(w);
                if (w.onChangeId != NullStringId && scriptCallback_) {
                    const char* fn = strings_.Lookup(w.onChangeId);
                    if (fn && fn[0] != '\0') scriptCallback_(fn);
                }
                handled = true;
                break;
            case WidgetTag::RadioButton:
                if (!w.checked) {
                    w.checked = true;
                    // Deselect siblings in the same radio group
                    if (w.radioGroup != NullStringId && w.parent >= 0) {
                        Widget* p = pool_.Get(w.parent);
                        if (p) {
                            for (int ci = 0; ci < p->childCount; ++ci) {
                                Widget* sib = pool_.Get(p->children[ci]);
                                if (sib && sib != &w &&
                                    sib->tag == WidgetTag::RadioButton &&
                                    sib->radioGroup == w.radioGroup) {
                                    sib->checked = false;
                                }
                            }
                        }
                    }
                    if (w.onChangeCpp) w.onChangeCpp(w);
                    if (w.onChangeId != NullStringId && scriptCallback_) {
                        const char* fn = strings_.Lookup(w.onChangeId);
                        if (fn && fn[0] != '\0') scriptCallback_(fn);
                    }
                }
                handled = true;
                break;
            case WidgetTag::NumberSpinner: {
                float midX = w.computedRect.x + w.computedRect.w * 0.5f;
                if (lastPointerX_ < midX)
                    w.sliderValue = std::max(w.sliderMin, w.sliderValue - w.spinnerStep);
                else
                    w.sliderValue = std::min(w.sliderMax, w.sliderValue + w.spinnerStep);
                if (w.onChangeCpp) w.onChangeCpp(w);
                if (w.onChangeId != NullStringId && scriptCallback_) {
                    const char* fn = strings_.Lookup(w.onChangeId);
                    if (fn && fn[0] != '\0') scriptCallback_(fn);
                }
                handled = true;
                break;
            }
            case WidgetTag::MenuItem:
                DismissPopup();
                handled = false; // still dispatch onClick
                break;
            default:
                break;
        }

        // Dispatch onClick on target
        if (w.onClickCpp) { w.onClickCpp(w); handled = true; }
        if (w.onClickId != NullStringId && scriptCallback_) {
            const char* fnName = strings_.Lookup(w.onClickId);
            if (fnName && fnName[0] != '\0') { scriptCallback_(fnName); handled = true; }
        }

        // Event bubbling: walk up parent chain if not handled
        if (!handled) {
            int parentIdx = w.parent;
            while (parentIdx >= 0) {
                Widget* parent = pool_.Get(parentIdx);
                if (!parent) break;

                if (parent->onClickCpp) { parent->onClickCpp(*parent); break; }
                if (parent->onClickId != NullStringId && scriptCallback_) {
                    const char* fnName = strings_.Lookup(parent->onClickId);
                    if (fnName && fnName[0] != '\0') { scriptCallback_(fnName); break; }
                }
                parentIdx = parent->parent;
            }
        }
    }

    ScriptCallbackFn scriptCallback_;
    WidgetPool pool_;
    StringTable strings_;
    Theme theme_;
    LayoutEngine layout_;
    FocusManager focus_;

    int rootIndex_ = -1;
    int focusedWidget_ = -1;
    int hoveredWidget_ = -1;

    // ---- Selection state ----
    std::vector<int> selectedNodes_;
    int selectionAnchor_ = -1;
    Modifiers lastModifiers_{};

    // ---- Tooltip state ----
    float tooltipHoverTime_ = 0.0f;
    static constexpr float TOOLTIP_DELAY = 0.5f; // seconds before showing
    int tooltipWidget_ = -1;
    float tooltipX_ = 0.0f;
    float tooltipY_ = 0.0f;

    // ---- Context menu state ----
    int activePopup_ = -1;  // pool index of currently visible popup menu

    // ---- Smooth scroll state ----
    SmoothScroll smoothScrolls_[MAX_SMOOTH_SCROLLS]{};

    float viewportW_ = 1920.0f;
    float viewportH_ = 1080.0f;

    // ---- Last pointer position (for click dispatch) ----
    float lastPointerX_ = 0.0f;
    float lastPointerY_ = 0.0f;

    // ---- Divider resize drag state ----
    static constexpr float DIVIDER_HIT_HALF = 4.0f;  // px each side of border
    static constexpr float MIN_PANEL_SIZE = 40.0f;

    struct ResizeDrag {
        bool active = false;
        int parentIdx = -1;      // parent container being resized
        int childA = -1;         // left/top child index in pool
        int childB = -1;         // right/bottom child index in pool
        float startMouse = 0.0f; // mouse position at drag start
        float startSizeA = 0.0f; // childA size at drag start
        float startSizeB = 0.0f; // childB size at drag start
        bool horizontal = true;  // true=dragging X (row), false=dragging Y (col)

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

    struct ScrollDrag {
        bool active = false;
        int scrollViewIdx = -1;
        float startMouseY = 0.0f;
        float startScrollY = 0.0f;
    } scrollDrag_;

    // ---- Floating panel drag state ----
    enum class PanelDragMode : uint8_t { None, Move, ResizeLeft, ResizeRight, ResizeTop, ResizeBottom,
                                         ResizeTL, ResizeTR, ResizeBL, ResizeBR };
    struct PanelDrag {
        bool active = false;
        int panelIdx = -1;
        PanelDragMode mode = PanelDragMode::None;
        float startMouseX = 0.0f;
        float startMouseY = 0.0f;
        float startPanelX = 0.0f;
        float startPanelY = 0.0f;
        float startPanelW = 0.0f;
        float startPanelH = 0.0f;
    } panelDrag_;

    /// Check if (px,py) hits a floating panel's title bar or resize edge.
    bool FindFloatingPanelHit(float px, float py) {
        // Check floating panels in reverse z-order (front to back)
        int bestIdx = -1;
        int16_t bestZ = -32768;
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            Widget* w = pool_.Get(static_cast<int>(i));
            if (!w || !pool_.IsAlive(static_cast<int>(i))) continue;
            if (w->tag != WidgetTag::FloatingPanel || !w->flags.visible || w->flags.minimized) continue;
            Rect r = w->computedRect;
            float edge = Widget::RESIZE_EDGE;
            Rect expanded = {r.x - edge, r.y - edge, r.w + edge * 2, r.h + edge * 2};
            if (expanded.Contains(px, py) && w->zOrder > bestZ) {
                bestIdx = static_cast<int>(i);
                bestZ = w->zOrder;
            }
        }
        if (bestIdx < 0) return false;

        Widget* panel = pool_.Get(bestIdx);
        Rect r = panel->computedRect;
        float edge = Widget::RESIZE_EDGE;
        float tbH = Widget::TITLE_BAR_HEIGHT;

        // Determine drag mode
        bool onLeft   = px < r.x + edge;
        bool onRight  = px > r.x + r.w - edge;
        bool onTop    = py < r.y + edge;
        bool onBottom = py > r.y + r.h - edge;

        PanelDragMode mode = PanelDragMode::None;
        if (onTop && onLeft)       mode = PanelDragMode::ResizeTL;
        else if (onTop && onRight) mode = PanelDragMode::ResizeTR;
        else if (onBottom && onLeft)  mode = PanelDragMode::ResizeBL;
        else if (onBottom && onRight) mode = PanelDragMode::ResizeBR;
        else if (onLeft)   mode = PanelDragMode::ResizeLeft;
        else if (onRight)  mode = PanelDragMode::ResizeRight;
        else if (onTop)    mode = PanelDragMode::ResizeTop;
        else if (onBottom) mode = PanelDragMode::ResizeBottom;
        else if (py < r.y + tbH) {
            float btnSize = tbH - 6.0f;
            // Close button (X) - rightmost in title bar
            float closeBtnX = r.x + r.w - btnSize - 4.0f;
            float btnY = r.y + 3.0f;
            if (px >= closeBtnX && px <= closeBtnX + btnSize && py >= btnY && py <= btnY + btnSize) {
                SetVisible(bestIdx, false);
                return true;
            }
            // Minimize button (-) - left of close button
            float minBtnX = closeBtnX - btnSize - 4.0f;
            if (px >= minBtnX && px <= minBtnX + btnSize && py >= btnY && py <= btnY + btnSize) {
                MinimizePanel(bestIdx);
                return true;
            }
            mode = PanelDragMode::Move;
        }

        if (mode == PanelDragMode::None) return false;

        panelDrag_.active = true;
        panelDrag_.panelIdx = bestIdx;
        panelDrag_.mode = mode;
        panelDrag_.startMouseX = px;
        panelDrag_.startMouseY = py;
        panelDrag_.startPanelX = panel->posLeft;
        panelDrag_.startPanelY = panel->posTop;
        panelDrag_.startPanelW = panel->localW;
        panelDrag_.startPanelH = panel->localH;

        BringToFront(bestIdx);
        return true;
    }

    void HandlePanelDrag(float px, float py) {
        Widget* panel = pool_.Get(panelDrag_.panelIdx);
        if (!panel) { panelDrag_.active = false; return; }

        float dx = px - panelDrag_.startMouseX;
        float dy = py - panelDrag_.startMouseY;

        switch (panelDrag_.mode) {
        case PanelDragMode::Move:
            panel->posLeft = panelDrag_.startPanelX + dx;
            panel->posTop  = panelDrag_.startPanelY + dy;
            break;
        case PanelDragMode::ResizeRight:
            panel->localW = std::max(panel->minWidth, panelDrag_.startPanelW + dx);
            break;
        case PanelDragMode::ResizeBottom:
            panel->localH = std::max(panel->minHeight, panelDrag_.startPanelH + dy);
            break;
        case PanelDragMode::ResizeLeft: {
            float newW = std::max(panel->minWidth, panelDrag_.startPanelW - dx);
            panel->posLeft = panelDrag_.startPanelX + (panelDrag_.startPanelW - newW);
            panel->localW = newW;
            break;
        }
        case PanelDragMode::ResizeTop: {
            float newH = std::max(panel->minHeight, panelDrag_.startPanelH - dy);
            panel->posTop = panelDrag_.startPanelY + (panelDrag_.startPanelH - newH);
            panel->localH = newH;
            break;
        }
        case PanelDragMode::ResizeTL: {
            float newW = std::max(panel->minWidth, panelDrag_.startPanelW - dx);
            float newH = std::max(panel->minHeight, panelDrag_.startPanelH - dy);
            panel->posLeft = panelDrag_.startPanelX + (panelDrag_.startPanelW - newW);
            panel->posTop  = panelDrag_.startPanelY + (panelDrag_.startPanelH - newH);
            panel->localW = newW;
            panel->localH = newH;
            break;
        }
        case PanelDragMode::ResizeTR: {
            float newH = std::max(panel->minHeight, panelDrag_.startPanelH - dy);
            panel->posTop = panelDrag_.startPanelY + (panelDrag_.startPanelH - newH);
            panel->localW = std::max(panel->minWidth, panelDrag_.startPanelW + dx);
            panel->localH = newH;
            break;
        }
        case PanelDragMode::ResizeBL: {
            float newW = std::max(panel->minWidth, panelDrag_.startPanelW - dx);
            panel->posLeft = panelDrag_.startPanelX + (panelDrag_.startPanelW - newW);
            panel->localW = newW;
            panel->localH = std::max(panel->minHeight, panelDrag_.startPanelH + dy);
            break;
        }
        case PanelDragMode::ResizeBR:
            panel->localW = std::max(panel->minWidth, panelDrag_.startPanelW + dx);
            panel->localH = std::max(panel->minHeight, panelDrag_.startPanelH + dy);
            break;
        default: break;
        }
        MarkDirty(panelDrag_.panelIdx);
    }

    /// Returns the appropriate cursor type if the mouse is over a floating panel edge.
    CursorType GetFloatingPanelEdgeCursor(float px, float py) const {
        int bestIdx = -1;
        int16_t bestZ = -32768;
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            const Widget* w = pool_.Get(static_cast<int>(i));
            if (!w || !pool_.IsAlive(static_cast<int>(i))) continue;
            if (w->tag != WidgetTag::FloatingPanel || !w->flags.visible || w->flags.minimized) continue;
            Rect r = w->computedRect;
            float edge = Widget::RESIZE_EDGE;
            Rect expanded = {r.x - edge, r.y - edge, r.w + edge * 2, r.h + edge * 2};
            if (expanded.Contains(px, py) && w->zOrder > bestZ) {
                bestIdx = static_cast<int>(i);
                bestZ = w->zOrder;
            }
        }
        if (bestIdx < 0) return CursorType::Default;

        const Widget* panel = pool_.Get(bestIdx);
        Rect r = panel->computedRect;
        float edge = Widget::RESIZE_EDGE;

        bool onLeft   = px < r.x + edge;
        bool onRight  = px > r.x + r.w - edge;
        bool onTop    = py < r.y + edge;
        bool onBottom = py > r.y + r.h - edge;

        if ((onTop && onLeft) || (onBottom && onRight)) return CursorType::NWSEResize;
        if ((onTop && onRight) || (onBottom && onLeft)) return CursorType::NESWResize;
        if (onLeft || onRight) return CursorType::EWResize;
        if (onTop || onBottom) return CursorType::NSResize;
        return CursorType::Default;
    }

    /// Minimize a floating panel to the status bar tray.
    void MinimizePanel(int panelIdx) {
        Widget* w = pool_.Get(panelIdx);
        if (!w || w->tag != WidgetTag::FloatingPanel) return;
        w->flags.minimized = 1;
        w->flags.visible = 0;
        MarkDirty(panelIdx);
    }

    /// Restore a minimized floating panel.
    void RestorePanel(int panelIdx) {
        Widget* w = pool_.Get(panelIdx);
        if (!w || w->tag != WidgetTag::FloatingPanel) return;
        w->flags.minimized = 0;
        w->flags.visible = 1;
        BringToFront(panelIdx);
        MarkDirty(panelIdx);
    }

    /// Get all minimized floating panels (for tray rendering).
    std::vector<int> GetMinimizedPanels() const {
        std::vector<int> result;
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            const Widget* w = pool_.Get(static_cast<int>(i));
            if (w && pool_.IsAlive(static_cast<int>(i))
                && w->tag == WidgetTag::FloatingPanel && w->flags.minimized) {
                result.push_back(static_cast<int>(i));
            }
        }
        return result;
    }

    /// Check if click hit a minimized panel tray button; restore if so.
    bool FindMinimizedTrayHit(float px, float py) {
        auto panels = GetMinimizedPanels();
        if (panels.empty()) return false;

        float btnH = 22.0f;
        float btnPad = 6.0f;
        float btnGap = 4.0f;
        float fontSize = 11.0f;
        float charW = fontSize * 0.55f;
        float vpW = viewportW_;
        float vpH = viewportH_;

        float totalW = 0.0f;
        std::vector<float> widths;
        for (int idx : panels) {
            const Widget* w = pool_.Get(idx);
            const char* title = w ? strings_.Lookup(w->panelTitleId) : nullptr;
            int len = title ? static_cast<int>(strlen(title)) : 0;
            float bw = std::max(60.0f, len * charW + btnPad * 2.0f);
            widths.push_back(bw);
            totalW += bw + btnGap;
        }
        totalW -= btnGap;

        float startX = (vpW - totalW) * 0.5f;
        float trayY = vpH - 26.0f - btnH - 4.0f;

        if (py < trayY || py > trayY + btnH) return false;

        float cx = startX;
        for (size_t i = 0; i < panels.size(); ++i) {
            if (px >= cx && px <= cx + widths[i]) {
                RestorePanel(panels[i]);
                return true;
            }
            cx += widths[i] + btnGap;
        }
        return false;
    }

    /// Check if (px,py) is on a divider between children of a layout container.
    /// Returns true and fills drag state if hit.
    bool FindDividerHit(float px, float py) {
        // Check dock containers and split panes for divider regions
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            Widget* parent = pool_.Get(static_cast<int>(i));
            if (!parent || !pool_.IsAlive(static_cast<int>(i))) continue;
            if (!parent->flags.visible || parent->childCount < 2) continue;

            // Only resizable containers
            if (!parent->flags.resizable) continue;

            bool isRow = (parent->layout.direction == LayoutDirection::Row);

            // Check dividers between consecutive visible children
            int prevChild = -1;
            for (int c = 0; c < parent->childCount; ++c) {
                Widget* child = pool_.Get(parent->children[c]);
                if (!child || !child->flags.visible) continue;

                if (prevChild >= 0) {
                    Widget* childA = pool_.Get(prevChild);
                    Widget* childB = child;
                    if (!childA) { prevChild = parent->children[c]; continue; }

                    float dividerPos, mousePos;
                    if (isRow) {
                        dividerPos = childA->computedRect.x + childA->computedRect.w;
                        mousePos = px;
                        // Check Y bounds
                        float top = std::min(childA->computedRect.y, childB->computedRect.y);
                        float bot = std::max(childA->computedRect.y + childA->computedRect.h,
                                             childB->computedRect.y + childB->computedRect.h);
                        if (py < top || py > bot) { prevChild = parent->children[c]; continue; }
                    } else {
                        dividerPos = childA->computedRect.y + childA->computedRect.h;
                        mousePos = py;
                        // Check X bounds
                        float left = std::min(childA->computedRect.x, childB->computedRect.x);
                        float right = std::max(childA->computedRect.x + childA->computedRect.w,
                                               childB->computedRect.x + childB->computedRect.w);
                        if (px < left || px > right) { prevChild = parent->children[c]; continue; }
                    }

                    if (mousePos >= dividerPos - DIVIDER_HIT_HALF &&
                        mousePos <= dividerPos + DIVIDER_HIT_HALF + parent->layout.spacing) {
                        resizeDrag_.parentIdx = static_cast<int>(i);
                        resizeDrag_.childA = prevChild;
                        resizeDrag_.childB = parent->children[c];
                        resizeDrag_.horizontal = isRow;
                        resizeDrag_.startMouse = isRow ? px : py;
                        resizeDrag_.startSizeA = isRow ? childA->computedRect.w : childA->computedRect.h;
                        resizeDrag_.startSizeB = isRow ? childB->computedRect.w : childB->computedRect.h;
                        return true;
                    }
                }
                prevChild = parent->children[c];
            }
        }
        return false;
    }

    void HandleResizeDrag(float px, float py) {
        if (!resizeDrag_.active) return;

        Widget* childA = pool_.Get(resizeDrag_.childA);
        Widget* childB = pool_.Get(resizeDrag_.childB);
        if (!childA || !childB) { resizeDrag_.active = false; return; }

        float mousePos = resizeDrag_.horizontal ? px : py;
        float delta = mousePos - resizeDrag_.startMouse;

        float newSizeA = resizeDrag_.startSizeA + delta;
        float newSizeB = resizeDrag_.startSizeB - delta;

        // Enforce minimum sizes
        if (newSizeA < MIN_PANEL_SIZE) {
            newSizeA = MIN_PANEL_SIZE;
            newSizeB = resizeDrag_.startSizeA + resizeDrag_.startSizeB - MIN_PANEL_SIZE;
        }
        if (newSizeB < MIN_PANEL_SIZE) {
            newSizeB = MIN_PANEL_SIZE;
            newSizeA = resizeDrag_.startSizeA + resizeDrag_.startSizeB - MIN_PANEL_SIZE;
        }

        // Apply - set Fixed size mode on the resized axis
        SizeMode modeA = resizeDrag_.horizontal ? childA->widthMode : childA->heightMode;
        SizeMode modeB = resizeDrag_.horizontal ? childB->widthMode : childB->heightMode;

        if (modeA == SizeMode::FillRemaining && modeB == SizeMode::FillRemaining) {
            // Both are fill - convert both to fixed
            if (resizeDrag_.horizontal) {
                childA->localW = newSizeA;
                childA->widthMode = SizeMode::Fixed;
                childB->localW = newSizeB;
                childB->widthMode = SizeMode::Fixed;
            } else {
                childA->localH = newSizeA;
                childA->heightMode = SizeMode::Fixed;
                childB->localH = newSizeB;
                childB->heightMode = SizeMode::Fixed;
            }
        } else if (modeA == SizeMode::FillRemaining) {
            // Only B is fixed - adjust B, A fills remainder
            if (resizeDrag_.horizontal)
                childB->localW = newSizeB;
            else
                childB->localH = newSizeB;
        } else if (modeB == SizeMode::FillRemaining) {
            // Only A is fixed - adjust A, B fills remainder
            if (resizeDrag_.horizontal)
                childA->localW = newSizeA;
            else
                childA->localH = newSizeA;
        } else {
            // Both fixed - adjust both
            if (resizeDrag_.horizontal) {
                childA->localW = newSizeA;
                childB->localW = newSizeB;
            } else {
                childA->localH = newSizeA;
                childB->localH = newSizeB;
            }
        }

        MarkDirty(resizeDrag_.parentIdx);
    }

    /// Check if (px,py) hits a scrollbar track/thumb on any ScrollView.
    bool FindScrollbarHit(float px, float py) {
        constexpr float BAR_WIDTH = 8.0f;
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            Widget* w = pool_.Get(static_cast<int>(i));
            if (!w || !pool_.IsAlive(static_cast<int>(i))) continue;
            if (w->tag != WidgetTag::ScrollView && !w->flags.scrollable) continue;
            if (!w->flags.visible) continue;
            if (w->contentHeight <= w->computedRect.h) continue; // no scrollbar

            const Rect& r = w->computedRect;
            float trackX = r.x + r.w - BAR_WIDTH;
            if (px >= trackX && px <= r.x + r.w &&
                py >= r.y && py <= r.y + r.h) {
                scrollDrag_.active = true;
                scrollDrag_.scrollViewIdx = static_cast<int>(i);
                scrollDrag_.startMouseY = py;
                scrollDrag_.startScrollY = w->scrollY;
                return true;
            }
        }
        return false;
    }

    void HandleScrollDrag(float py) {
        if (!scrollDrag_.active) return;
        Widget* sv = pool_.Get(scrollDrag_.scrollViewIdx);
        if (!sv) { scrollDrag_.active = false; return; }

        float trackH = sv->computedRect.h;
        float contentH = sv->contentHeight;
        float maxScroll = contentH - trackH;
        if (maxScroll <= 0.0f) { scrollDrag_.active = false; return; }

        // Mouse delta in track space maps to scroll delta in content space
        float mouseDelta = py - scrollDrag_.startMouseY;
        float scrollDelta = mouseDelta * (contentH / trackH);
        sv->scrollY = scrollDrag_.startScrollY - scrollDelta;

        // Clamp
        if (sv->scrollY > 0.0f) sv->scrollY = 0.0f;
        if (sv->scrollY < -maxScroll) sv->scrollY = -maxScroll;
        MarkDirty(scrollDrag_.scrollViewIdx);
    }

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
