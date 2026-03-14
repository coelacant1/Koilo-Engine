// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_context.cpp
 * @brief UIContext implementation - widget creation, event dispatch, and layout.
 * @date 03/08/2026
 * @author Coela Can't
 */
#include "ui_context.hpp"
#include "color_picker.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace koilo {
namespace ui {

// Destructor must be here so unique_ptr<ColorPicker> can see the full type.
UIContext::~UIContext() = default;

// Constructor (out-of-line for same reason as destructor).
UIContext::UIContext(size_t poolCapacity)
    : pool_(poolCapacity) {}

// ============================================================================
// Widget Creation
// ============================================================================

// Create a widget and return its pool index.
int UIContext::CreateWidget(WidgetTag tag, const char* id) {
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

// Create a label widget.
int UIContext::CreateLabel(const char* id, const char* text) {
    int idx = CreateWidget(WidgetTag::Label, id);
    if (idx >= 0) {
        pool_.Get(idx)->textId = strings_.Intern(text);
    }
    return idx;
}

// Create a button widget.
int UIContext::CreateButton(const char* id, const char* text) {
    int idx = CreateWidget(WidgetTag::Button, id);
    if (idx >= 0) {
        pool_.Get(idx)->textId = strings_.Intern(text);
    }
    return idx;
}

// Create a slider widget.
int UIContext::CreateSlider(const char* id, float min, float max, float value) {
    int idx = CreateWidget(WidgetTag::Slider, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->sliderMin = min;
        w->sliderMax = max;
        w->sliderValue = value;
    }
    return idx;
}

// Create a checkbox widget.
int UIContext::CreateCheckbox(const char* id, bool checked) {
    int idx = CreateWidget(WidgetTag::Checkbox, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->checked = checked;
        w->localW = 16.0f;
        w->localH = 16.0f;
    }
    return idx;
}

// Create a text field widget.
int UIContext::CreateTextField(const char* id, const char* placeholder) {
    int idx = CreateWidget(WidgetTag::TextField, id);
    if (idx >= 0) {
        pool_.Get(idx)->placeholderId = strings_.Intern(placeholder);
    }
    return idx;
}

// Create a separator widget.
int UIContext::CreateSeparator(const char* id) {
    int idx = CreateWidget(WidgetTag::Separator, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->localH = 1.0f;
        w->widthMode = SizeMode::FillRemaining;
    }
    return idx;
}

// Create a progress bar widget.
int UIContext::CreateProgressBar(const char* id, float value) {
    int idx = CreateWidget(WidgetTag::ProgressBar, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->progressValue = value;
        w->localH = 8.0f;
        w->widthMode = SizeMode::FillRemaining;
    }
    return idx;
}

// Create a toggle switch widget.
int UIContext::CreateToggleSwitch(const char* id, bool on) {
    int idx = CreateWidget(WidgetTag::ToggleSwitch, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->checked = on;
        w->localW = 36.0f;
        w->localH = 20.0f;
    }
    return idx;
}

// Create a radio button widget.
int UIContext::CreateRadioButton(const char* id, const char* group, bool selected) {
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

// Create a number spinner widget.
int UIContext::CreateNumberSpinner(const char* id, float value, float min,
                                   float max, float step) {
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

// Create a virtual list widget.
int UIContext::CreateVirtualList(const char* id, int itemCount, float itemH,
                                 std::function<void(Widget&, int)> bind) {
    int idx = CreateWidget(WidgetTag::VirtualList, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->virtualItemCount = itemCount;
        w->virtualItemHeight = itemH;
        w->virtualBind = std::move(bind);
        w->widthMode = SizeMode::FillRemaining;
        w->heightMode = SizeMode::FillRemaining;
        w->flags.clipChildren = 1;
        w->flags.scrollable = 1;
        w->layout.direction = LayoutDirection::Column;
        w->contentHeight = itemCount * itemH;
    }
    return idx;
}

// Update the visible items in a VirtualList based on scroll position.
void UIContext::UpdateVirtualList(int listIdx) {
    Widget* list = pool_.Get(listIdx);
    if (!list || list->tag != WidgetTag::VirtualList) return;
    if (!list->virtualBind) return;

    float viewH = list->computedRect.h;
    float totalH = list->virtualItemCount * list->virtualItemHeight;
    list->contentHeight = totalH;
    float scrollY = list->scrollY;
    if (scrollY < 0.0f) scrollY = 0.0f;
    if (scrollY > totalH - viewH) scrollY = totalH - viewH;
    if (scrollY < 0.0f) scrollY = 0.0f;
    list->scrollY = scrollY;

    int firstVisible = static_cast<int>(scrollY / list->virtualItemHeight);
    int visibleCount = static_cast<int>(viewH / list->virtualItemHeight) + 2;
    if (firstVisible < 0) firstVisible = 0;
    if (firstVisible + visibleCount > list->virtualItemCount)
        visibleCount = list->virtualItemCount - firstVisible;

    // Re-bind existing child widgets (pool reuse)
    for (int i = 0; i < visibleCount && i < list->childCount; ++i) {
        Widget* child = pool_.Get(list->children[i]);
        if (!child) continue;
        int dataIdx = firstVisible + i;
        child->posTop = dataIdx * list->virtualItemHeight - scrollY;
        child->localH = list->virtualItemHeight;
        child->flags.visible = true;
        list->virtualBind(*child, dataIdx);
    }

    // Create more child widgets if needed
    while (list->childCount < visibleCount && list->childCount < MAX_CHILDREN) {
        char childId[64];
        snprintf(childId, 64, "vl_%d_item_%d", listIdx, list->childCount);
        int ci = CreateWidget(WidgetTag::Label, childId);
        if (ci < 0) break;
        SetParent(ci, listIdx);
        Widget* child = pool_.Get(ci);
        if (!child) break;
        int dataIdx = firstVisible + list->childCount - 1;
        child->positionMode = PositionMode::Absolute;
        child->posTop = dataIdx * list->virtualItemHeight - scrollY;
        child->localH = list->virtualItemHeight;
        child->widthMode = SizeMode::FillRemaining;
        child->flags.visible = true;
        list->virtualBind(*child, dataIdx);
    }

    // Hide excess children
    for (int i = visibleCount; i < list->childCount; ++i) {
        Widget* child = pool_.Get(list->children[i]);
        if (child) child->flags.visible = false;
    }

    MarkDirty(listIdx);
}

// Create a Canvas2D widget for custom drawing.
int UIContext::CreateCanvas2D(const char* id,
                               std::function<void(void*)> paintFn) {
    int idx = CreateWidget(WidgetTag::Canvas2D, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->widthMode = SizeMode::FillRemaining;
        w->heightMode = SizeMode::FillRemaining;
        w->flags.clipChildren = 1;
        w->flags.focusable = 1;
        if (paintFn) w->onPaint = std::move(paintFn);
    }
    return idx;
}

// Create a dock container widget.
int UIContext::CreateDockContainer(const char* id) {
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

// Create a split pane widget.
int UIContext::CreateSplitPane(const char* id, bool vertical) {
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

// Create a floating panel overlay.
int UIContext::CreateFloatingPanel(const char* id, const char* title,
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
    widget->padding.top = Widget::TITLE_BAR_HEIGHT;
    widget->flags.clipChildren = 1;
    widget->minWidth = 120.0f;
    widget->minHeight = 80.0f;
    return idx;
}

// Undock a panel from its parent and convert it to a floating panel.
int UIContext::UndockPanel(int panelIdx, float screenX, float screenY) {
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

// Re-dock a floating panel back to its original location.
bool UIContext::RedockPanel(int floatIdx) {
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

// Bring a floating panel to front (highest z-order).
void UIContext::BringToFront(int panelIdx) {
    Widget* panel = pool_.Get(panelIdx);
    if (!panel) return;
    int16_t maxZ = 500;
    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        const Widget* w = pool_.Get(static_cast<int>(i));
        if (w && w->tag == WidgetTag::FloatingPanel && static_cast<int>(i) != panelIdx) {
            if (w->zOrder >= maxZ) maxZ = w->zOrder + 1;
        }
    }
    panel->zOrder = maxZ;
}

// Create a tab bar widget.
int UIContext::CreateTabBar(const char* id) {
    int idx = CreateWidget(WidgetTag::TabBar, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->widthMode = SizeMode::FillRemaining;
        w->localH = 28.0f;
        w->layout.direction = LayoutDirection::Row;
    }
    return idx;
}

// Create a popup menu widget.
int UIContext::CreatePopupMenu(const char* id) {
    int idx = CreateWidget(WidgetTag::PopupMenu, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->flags.visible = false;
        w->layout.direction = LayoutDirection::Column;
    }
    return idx;
}

// Create a menu item widget.
int UIContext::CreateMenuItem(const char* id, const char* text) {
    int idx = CreateWidget(WidgetTag::MenuItem, id);
    if (idx >= 0) {
        Widget* w = pool_.Get(idx);
        w->textId = strings_.Intern(text);
        w->widthMode = SizeMode::FillRemaining;
        w->localH = 24.0f;
    }
    return idx;
}

// ============================================================================
// Tree Node API
// ============================================================================

// Create a tree node and attach it to a parent container.
int UIContext::AddTreeNode(int containerIdx, const char* id, const char* text,
                            int depth, bool hasChildren) {
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

// Remove a tree node and collapse its visual children.
void UIContext::RemoveTreeNode(int nodeIdx) {
    Widget* w = pool_.Get(nodeIdx);
    if (!w || w->tag != WidgetTag::TreeNode) return;
    int depth = w->treeDepth;
    int parentIdx = w->parent;

    if (parentIdx >= 0) {
        Widget* par = pool_.Get(parentIdx);
        if (par) {
            int myPos = -1;
            for (int c = 0; c < par->childCount; ++c) {
                if (par->children[c] == nodeIdx) { myPos = c; break; }
            }
            if (myPos >= 0) {
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

// Reparent a tree node to a new position.
void UIContext::ReparentTreeNode(int nodeIdx, int containerIdx, int insertAfterIdx, int newDepth) {
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

// Recalculate treeHasChildren for all tree nodes in a container.
void UIContext::RefreshTreeFlags(int containerIdx) {
    Widget* container = pool_.Get(containerIdx);
    if (!container) return;
    for (int c = 0; c < container->childCount; ++c) {
        Widget* w = pool_.Get(container->children[c]);
        if (!w || w->tag != WidgetTag::TreeNode) continue;
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

// Enable drag-to-reparent on a TreeNode widget.
void UIContext::EnableTreeNodeDragDrop(int nodeIdx) {
    Widget* w = pool_.Get(nodeIdx);
    if (!w || w->tag != WidgetTag::TreeNode) return;

    w->acceptDropTypes = (1u << DragType::Widget);

    w->onDragBegin = [this](int idx) -> DragPayload {
        Widget* src = pool_.Get(idx);
        if (!src) return {};
        DragPayload p;
        p.typeTag = DragType::Widget;
        p.sourceWidget = idx;
        p.labelId = src->textId;
        return p;
    };

    w->onCanDrop = [this](const DragPayload& payload) -> bool {
        return payload.typeTag == DragType::Widget && payload.sourceWidget >= 0;
    };

    w->onDrop = [this, nodeIdx](const DragPayload& payload, DropPosition pos) {
        if (payload.sourceWidget < 0) return;
        Widget* src = pool_.Get(payload.sourceWidget);
        Widget* tgt = pool_.Get(nodeIdx);
        if (!src || !tgt) return;
        if (payload.sourceWidget == nodeIdx) return;

        int containerIdx = tgt->parent;
        if (containerIdx < 0) return;

        if (pos == DropPosition::Into) {
            ReparentTreeNode(payload.sourceWidget, containerIdx,
                             nodeIdx, tgt->treeDepth + 1);
        } else if (pos == DropPosition::Before) {
            Widget* container = pool_.Get(containerIdx);
            if (!container) return;
            int insertAfter = -1;
            for (int c = 0; c < container->childCount; ++c) {
                if (container->children[c] == nodeIdx) break;
                insertAfter = container->children[c];
            }
            ReparentTreeNode(payload.sourceWidget, containerIdx,
                             insertAfter, tgt->treeDepth);
        } else {
            ReparentTreeNode(payload.sourceWidget, containerIdx,
                             nodeIdx, tgt->treeDepth);
        }
    };
}

// Enable drag-to-reparent on all TreeNode children in a container.
void UIContext::EnableTreeDragDrop(int containerIdx) {
    Widget* container = pool_.Get(containerIdx);
    if (!container) return;
    for (int c = 0; c < container->childCount; ++c) {
        Widget* child = pool_.Get(container->children[c]);
        if (child && child->tag == WidgetTag::TreeNode) {
            EnableTreeNodeDragDrop(container->children[c]);
        }
    }
}

// ============================================================================
// Selection API
// ============================================================================

// Select a single tree node, clearing any previous selection.
void UIContext::SelectNode(int nodeIdx) {
    ClearSelection();
    Widget* w = pool_.Get(nodeIdx);
    if (!w) return;
    w->flags.selected = 1;
    selectedNodes_.push_back(nodeIdx);
    theme_.NotifyStateChange(*w, nodeIdx);
}

// Toggle selection on a node (for Ctrl+Click).
void UIContext::ToggleSelectNode(int nodeIdx) {
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

// Range-select tree nodes between anchor and target (for Shift+Click).
void UIContext::RangeSelectNodes(int anchorIdx, int targetIdx) {
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

// Deselect a specific node.
void UIContext::DeselectNode(int nodeIdx) {
    Widget* w = pool_.Get(nodeIdx);
    if (!w) return;
    w->flags.selected = 0;
    selectedNodes_.erase(
        std::remove(selectedNodes_.begin(), selectedNodes_.end(), nodeIdx),
        selectedNodes_.end());
    theme_.NotifyStateChange(*w, nodeIdx);
}

// Clear all selection.
void UIContext::ClearSelection() {
    for (int idx : selectedNodes_) {
        Widget* w = pool_.Get(idx);
        if (w) {
            w->flags.selected = 0;
            theme_.NotifyStateChange(*w, idx);
        }
    }
    selectedNodes_.clear();
}

// ============================================================================
// Widget Tree Management
// ============================================================================

// Attach child to parent.
bool UIContext::SetParent(int childIdx, int parentIdx) {
    Widget* child = pool_.Get(childIdx);
    Widget* parent = pool_.Get(parentIdx);
    if (!child || !parent) return false;
    if (parent->childCount >= MAX_CHILDREN) return false;

    if (child->parent >= 0) {
        Widget* oldParent = pool_.Get(child->parent);
        if (oldParent) RemoveChild(*oldParent, childIdx);
    }

    parent->children[parent->childCount++] = static_cast<int16_t>(childIdx);
    child->parent = static_cast<int16_t>(parentIdx);

    // Dropdown children are not directly clickable; clicks go to the dropdown.
    if (parent->tag == WidgetTag::Dropdown) {
        child->flags.pointerEvents = 0;
        child->flags.visible = false;  // Hidden; rendered inline by EmitDropdown.
    }

    MarkDirty(parentIdx);
    return true;
}

// Destroy a widget and all its children recursively.
void UIContext::DestroyWidget(int idx) {
    Widget* w = pool_.Get(idx);
    if (!w) return;

    while (w->childCount > 0) {
        DestroyWidget(w->children[w->childCount - 1]);
    }

    if (w->parent >= 0) {
        Widget* p = pool_.Get(w->parent);
        if (p) RemoveChild(*p, idx);
    }

    if (focusedWidget_ == idx) focusedWidget_ = -1;
    pool_.Free(idx);
}

// ============================================================================
// Property Setters
// ============================================================================

// Set layout properties on a widget.
void UIContext::SetLayout(int idx, LayoutDirection dir, Alignment mainAlign,
                           Alignment crossAlign, float spacing) {
    Widget* w = pool_.Get(idx);
    if (!w) return;
    w->layout.direction = dir;
    w->layout.mainAlign = mainAlign;
    w->layout.crossAlign = crossAlign;
    w->layout.spacing = spacing;
    MarkDirty(idx);
}

// Set padding on a widget.
void UIContext::SetPadding(int idx, float top, float right, float bottom, float left) {
    Widget* w = pool_.Get(idx);
    if (!w) return;
    w->padding = {top, right, bottom, left};
    MarkDirty(idx);
}

// Set margin on a widget.
void UIContext::SetMargin(int idx, float top, float right, float bottom, float left) {
    Widget* w = pool_.Get(idx);
    if (!w) return;
    w->margin = {top, right, bottom, left};
    MarkDirty(idx);
}

// Set anchors on a widget.
void UIContext::SetAnchors(int idx, float minX, float minY, float maxX, float maxY) {
    Widget* w = pool_.Get(idx);
    if (!w) return;
    w->anchors = {minX, minY, maxX, maxY};
    MarkDirty(idx);
}

// Set size mode on a widget.
void UIContext::SetSizeMode(int idx, SizeMode wMode, SizeMode hMode) {
    Widget* w = pool_.Get(idx);
    if (!w) return;
    w->widthMode = wMode;
    w->heightMode = hMode;
    MarkDirty(idx);
}

// ============================================================================
// Update Cycle
// ============================================================================

// Set the viewport size.
void UIContext::SetViewport(float width, float height) {
    if (viewportW_ != width || viewportH_ != height) {
        viewportW_ = width;
        viewportH_ = height;
        if (rootIndex_ >= 0) MarkDirty(rootIndex_);
    }
}

// Run layout pass.
void UIContext::UpdateLayout() {
    if (rootIndex_ < 0) return;
    Widget* root = pool_.Get(rootIndex_);
    if (!root) return;

    root->computedRect = {0.0f, 0.0f, viewportW_, viewportH_};
    layout_.Compute(pool_, rootIndex_);
}

// Process a raw input event.
void UIContext::ProcessEvent(Event event) {
    if (rootIndex_ < 0) return;

    switch (event.type) {
        case EventType::PointerDown:
            if (event.pointerButton == 1) {
                HandleRightClick(event);
            } else {
                if (popupDepth_ > 0) {
                    bool insidePopup = false;
                    for (int pi = 0; pi < popupDepth_; ++pi) {
                        Widget* pp = pool_.Get(popupStack_[pi]);
                        if (pp && pp->computedRect.Contains(event.pointerX, event.pointerY)) {
                            insidePopup = true;
                            break;
                        }
                    }
                    if (!insidePopup) DismissPopup();
                }
                HandlePointerDown(event);
                // Route content-area clicks to color picker after panel drag check.
                if (colorPicker_ && colorPicker_->IsOpen() && !panelDrag_.active) {
                    colorPicker_->HandlePointerDown(event.pointerX, event.pointerY);
                }
            }
            break;
        case EventType::PointerUp:
            if (event.pointerButton == 0) {
                if (colorPicker_) colorPicker_->HandlePointerUp();
                HandlePointerUp(event);
            }
            break;
        case EventType::PointerMove:
            // Route active color picker canvas drag.
            if (colorPicker_ && colorPicker_->IsDragging()) {
                colorPicker_->HandlePointerDrag(event.pointerX, event.pointerY);
            }
            HandlePointerMove(event);
            break;
        case EventType::KeyDown:
            if (activeDrag_.IsActive() && event.key == KeyCode::Escape) {
                if (dragHoverTarget_ >= 0) {
                    Widget* prev = pool_.Get(dragHoverTarget_);
                    if (prev) { prev->flags.hovered = 0; theme_.NotifyStateChange(*prev, dragHoverTarget_); }
                }
                activeDrag_.Clear();
                dragCandidate_.widgetIdx = -1;
                dragHoverTarget_ = -1;
                for (size_t i = 0; i < pool_.Capacity(); ++i) {
                    Widget* w = pool_.Get(static_cast<int>(i));
                    if (w && w->flags.pressed) {
                        w->flags.pressed = 0;
                        theme_.NotifyStateChange(*w, static_cast<int>(i));
                    }
                }
            } else if (activePopup_ >= 0 && event.key == KeyCode::Escape) {
                DismissTopPopup();
            } else {
                if (commands_ && commands_->DispatchShortcut(event.key, event.mods)) {
                    // Shortcut handled by command registry
                } else {
                    HandleKeyDown(event);
                }
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

// Update transition animations.
void UIContext::UpdateTransitions(float dt) {
    theme_.UpdateTransitions(dt);

    // Keep render dirty while transitions are animating
    if (theme_.HasActiveTransitions()) renderDirty_ = true;

    // Update tooltip timer
    int prevTooltip = tooltipWidget_;
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
    if (tooltipWidget_ != prevTooltip) renderDirty_ = true;

    // Update smooth scrolling
    UpdateSmoothScroll(dt);

    // Update sub-menu hover timer
    if (submenuHoverItem_ >= 0) {
        static constexpr float SUBMENU_HOVER_DELAY = 0.25f;
        submenuHoverTime_ += dt;
        if (submenuHoverTime_ >= SUBMENU_HOVER_DELAY) {
            Widget* item = pool_.Get(submenuHoverItem_);
            if (item && item->submenuIdx >= 0) {
                int parentPopup = item->parent;
                while (popupDepth_ > 0 && popupStack_[popupDepth_ - 1] != parentPopup) {
                    DismissTopPopup();
                }
                ShowSubMenu(submenuHoverItem_, item->submenuIdx);
            }
            submenuHoverItem_ = -1;
            submenuHoverTime_ = 0.0f;
        }
    }
}

// ============================================================================
// Query & Popup Management
// ============================================================================

// Get the cursor style for the currently hovered widget.
CursorType UIContext::GetRequestedCursor() const {
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

// Find a widget by interned string ID.
int UIContext::FindWidget(const char* id) const {
    StringId sid = const_cast<StringTable&>(strings_).Intern(id);
    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        const Widget* w = pool_.Get(static_cast<int>(i));
        if (w && w->id == sid) return static_cast<int>(i);
    }
    return -1;
}

// Show a popup at a screen position.
void UIContext::ShowPopup(int popupIdx, float x, float y) {
    Widget* popup = pool_.Get(popupIdx);
    if (!popup) return;
    popup->flags.visible = true;
    popup->positionMode = PositionMode::Absolute;
    popup->posLeft = x;
    popup->posTop = y;
    popup->zOrder = static_cast<int16_t>(1000 + popupDepth_);
    if (popupDepth_ < MAX_POPUP_DEPTH) {
        popupStack_[popupDepth_++] = popupIdx;
    }
    activePopup_ = popupIdx;
    MarkDirty(popupIdx);
}

// Dismiss the top popup only.
bool UIContext::DismissTopPopup() {
    if (popupDepth_ <= 0) return false;
    int idx = popupStack_[--popupDepth_];
    Widget* popup = pool_.Get(idx);
    if (popup) {
        popup->flags.visible = false;
        popup->zOrder = 0;
        MarkDirty(idx);
    }
    activePopup_ = (popupDepth_ > 0) ? popupStack_[popupDepth_ - 1] : -1;
    if (popupDepth_ == 0) menuBarOpen_ = false;
    return true;
}

// Dismiss all active popups.
void UIContext::DismissPopup() {
    while (popupDepth_ > 0) {
        DismissTopPopup();
    }
    submenuHoverItem_ = -1;
    submenuHoverTime_ = 0.0f;
}

// Show a sub-menu popup to the right of a MenuItem.
void UIContext::ShowSubMenu(int menuItemIdx, int submenuPopupIdx) {
    Widget* item = pool_.Get(menuItemIdx);
    Widget* sub = pool_.Get(submenuPopupIdx);
    if (!item || !sub) return;

    const Rect& ir = item->computedRect;
    float x = ir.x + ir.w;
    float y = ir.y;

    if (x + 150.0f > viewportW_) {
        Widget* parentPopup = pool_.Get(item->parent);
        float pw = parentPopup ? parentPopup->computedRect.w : 150.0f;
        x = ir.x - pw;
        if (x < 0.0f) x = 0.0f;
    }
    if (y + 200.0f > viewportH_) y = viewportH_ - 200.0f;
    if (y < 0.0f) y = 0.0f;

    ShowPopup(submenuPopupIdx, x, y);
}

// ============================================================================
// Internal Helpers (private)
// ============================================================================

// Find or create a smooth scroll slot for a widget.
UIContext::SmoothScroll* UIContext::FindOrCreateSmoothScroll(int widgetIdx) {
    for (auto& ss : smoothScrolls_)
        if (ss.active && ss.widgetIdx == widgetIdx) return &ss;
    for (auto& ss : smoothScrolls_) {
        if (!ss.active) {
            ss.widgetIdx = widgetIdx;
            return &ss;
        }
    }
    return nullptr;
}

// Update smooth scroll interpolation.
void UIContext::UpdateSmoothScroll(float dt) {
    constexpr float SMOOTH_SPEED = 12.0f;
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

// Remove a child from its parent.
void UIContext::RemoveChild(Widget& parent, int childIdx) {
    for (int i = 0; i < parent.childCount; ++i) {
        if (parent.children[i] == childIdx) {
            for (int j = i; j < parent.childCount - 1; ++j) {
                parent.children[j] = parent.children[j + 1];
            }
            parent.children[--parent.childCount] = -1;
            return;
        }
    }
}

// Set focus to a widget.
void UIContext::SetFocus(int idx, bool fromKeyboard) {
    if (focusedWidget_ == idx) return;
    Widget* old = pool_.Get(focusedWidget_);
    if (old) { old->flags.focused = 0; old->flags.focusVisible = 0; theme_.NotifyStateChange(*old, focusedWidget_); MarkDirty(focusedWidget_); }
    focusedWidget_ = idx;
    Widget* nw = pool_.Get(idx);
    if (nw && nw->flags.focusable) {
        nw->flags.focused = 1;
        nw->flags.focusVisible = fromKeyboard ? 1 : 0;
        theme_.NotifyStateChange(*nw, idx);
        MarkDirty(idx);
    }
}

// ============================================================================
// Event Handlers (private)
// ============================================================================

// Handle pointer down event.
void UIContext::HandlePointerDown(Event& event) {
    lastPointerX_ = event.pointerX;
    lastPointerY_ = event.pointerY;
    lastModifiers_ = event.mods;

    if (FindFloatingPanelHit(event.pointerX, event.pointerY)) {
        return;
    }

    if (FindMinimizedTrayHit(event.pointerX, event.pointerY)) {
        return;
    }

    if (FindDividerHit(event.pointerX, event.pointerY)) {
        resizeDrag_.active = true;
        return;
    }

    if (FindScrollbarHit(event.pointerX, event.pointerY)) {
        return;
    }

    int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);

    // Close any open dropdown that wasn't clicked
    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        Widget* dd = pool_.Get(static_cast<int>(i));
        if (dd && dd->tag == WidgetTag::Dropdown && dd->dropdownOpen) {
            if (static_cast<int>(i) != hit) {
                dd->dropdownOpen = false;
                MarkDirty(static_cast<int>(i));
            }
        }
    }

    if (hit >= 0) {
        Widget* w = pool_.Get(hit);
        if (w) {
            w->flags.pressed = 1;
            theme_.NotifyStateChange(*w, hit);
            if (w->flags.focusable) SetFocus(hit);

            if (w->onDragBegin) {
                dragCandidate_.widgetIdx = hit;
                dragCandidate_.startX = event.pointerX;
                dragCandidate_.startY = event.pointerY;
            }
        }
    } else {
        SetFocus(-1);
    }
}

// Handle pointer up event.
void UIContext::HandlePointerUp(Event& event) {
    if (panelDrag_.active) {
        panelDrag_.active = false;
        return;
    }
    if (resizeDrag_.active) {
        resizeDrag_.active = false;
        return;
    }
    if (scrollDrag_.active) {
        scrollDrag_.active = false;
        return;
    }

    if (activeDrag_.IsActive()) {
        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);
        Widget* target = pool_.Get(hit);
        if (target && CanAcceptDrop(*target, activeDrag_)) {
            if (target->onDrop) {
                target->onDrop(activeDrag_, dragDropPos_);
            }
        }
        if (dragHoverTarget_ >= 0) {
            Widget* prev = pool_.Get(dragHoverTarget_);
            if (prev) { prev->flags.hovered = 0; theme_.NotifyStateChange(*prev, dragHoverTarget_); }
        }
        activeDrag_.Clear();
        dragCandidate_.widgetIdx = -1;
        dragHoverTarget_ = -1;
        for (size_t i = 0; i < pool_.Capacity(); ++i) {
            Widget* w = pool_.Get(static_cast<int>(i));
            if (w && w->flags.pressed) {
                w->flags.pressed = 0;
                theme_.NotifyStateChange(*w, static_cast<int>(i));
            }
        }
        return;
    }

    dragCandidate_.widgetIdx = -1;

    int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);

    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        Widget* w = pool_.Get(static_cast<int>(i));
        if (w && w->flags.pressed) {
            w->flags.pressed = 0;
            theme_.NotifyStateChange(*w, static_cast<int>(i));
            if (static_cast<int>(i) == hit) {
                DispatchClick(*w);
            }
        }
    }
}

// Handle pointer move event.
void UIContext::HandlePointerMove(Event& event) {
    if (panelDrag_.active) {
        HandlePanelDrag(event.pointerX, event.pointerY);
        return;
    }
    if (resizeDrag_.active) {
        HandleResizeDrag(event.pointerX, event.pointerY);
        return;
    }
    if (scrollDrag_.active) {
        HandleScrollDrag(event.pointerY);
        return;
    }

    // General drag-and-drop: check threshold to initiate
    if (dragCandidate_.widgetIdx >= 0 && !activeDrag_.IsActive()) {
        float dx = event.pointerX - dragCandidate_.startX;
        float dy = event.pointerY - dragCandidate_.startY;
        if (dx * dx + dy * dy >= DRAG_THRESHOLD * DRAG_THRESHOLD) {
            Widget* src = pool_.Get(dragCandidate_.widgetIdx);
            if (src && src->onDragBegin) {
                activeDrag_ = src->onDragBegin(dragCandidate_.widgetIdx);
                if (!activeDrag_.IsActive()) {
                    dragCandidate_.widgetIdx = -1;
                }
            } else {
                dragCandidate_.widgetIdx = -1;
            }
        }
    }

    // General drag-and-drop: update hover target
    if (activeDrag_.IsActive()) {
        int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);
        if (hit != dragHoverTarget_) {
            if (dragHoverTarget_ >= 0) {
                Widget* prev = pool_.Get(dragHoverTarget_);
                if (prev) { prev->flags.hovered = 0; theme_.NotifyStateChange(*prev, dragHoverTarget_); }
            }
            dragHoverTarget_ = hit;
            Widget* target = pool_.Get(hit);
            if (target && CanAcceptDrop(*target, activeDrag_)) {
                target->flags.hovered = 1;
                theme_.NotifyStateChange(*target, hit);
            }
        }
        if (dragHoverTarget_ >= 0) {
            Widget* target = pool_.Get(dragHoverTarget_);
            if (target && target->tag == WidgetTag::TreeNode) {
                float relY = event.pointerY - target->computedRect.y;
                float h = target->computedRect.h;
                if (relY < h * 0.25f)       dragDropPos_ = DropPosition::Before;
                else if (relY > h * 0.75f)  dragDropPos_ = DropPosition::After;
                else                         dragDropPos_ = DropPosition::Into;
            } else {
                dragDropPos_ = DropPosition::Into;
            }
        }
        lastPointerX_ = event.pointerX;
        lastPointerY_ = event.pointerY;
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
        tooltipHoverTime_ = 0.0f;
        tooltipWidget_ = -1;
        renderDirty_ = true;
    }
    tooltipX_ = event.pointerX;
    tooltipY_ = event.pointerY;

    // Sub-menu hover
    Widget* hitW = pool_.Get(hit);
    if (hitW && hitW->tag == WidgetTag::MenuItem && hitW->submenuIdx >= 0) {
        if (hit != submenuHoverItem_) {
            submenuHoverItem_ = hit;
            submenuHoverTime_ = 0.0f;
        }
    } else if (submenuHoverItem_ >= 0) {
        bool inSubmenu = false;
        Widget* hoverItem = pool_.Get(submenuHoverItem_);
        if (hoverItem && hoverItem->submenuIdx >= 0) {
            Widget* sub = pool_.Get(hoverItem->submenuIdx);
            if (sub && sub->flags.visible && sub->computedRect.Contains(event.pointerX, event.pointerY)) {
                inSubmenu = true;
            }
        }
        if (!inSubmenu) {
            submenuHoverItem_ = -1;
            submenuHoverTime_ = 0.0f;
        }
    }

    // Menu bar hover-to-switch
    if (menuBarOpen_ && hitW && hitW->tag == WidgetTag::Button && popupDepth_ > 0) {
        Widget* btn = hitW;
        int btnParent = btn->parent;
        Widget* currentPopup = pool_.Get(popupStack_[0]);
        if (currentPopup && btnParent >= 0) {
            Widget* par = pool_.Get(btnParent);
            if (par) {
                for (int i = 0; i < par->childCount; ++i) {
                    Widget* sib = pool_.Get(par->children[i]);
                    if (sib && sib->tag == WidgetTag::PopupMenu && par->children[i] != popupStack_[0]) {
                        if (i > 0 && par->children[i-1] == hit) {
                            DismissPopup();
                            menuBarOpen_ = true;
                            float px = btn->computedRect.x;
                            float py = btn->computedRect.y + btn->computedRect.h;
                            ShowPopup(par->children[i], px, py);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Slider drag
    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        Widget* w = pool_.Get(static_cast<int>(i));
        if (w && w->flags.pressed && w->tag == WidgetTag::Slider) {
            float relX = event.pointerX - w->computedRect.x;
            float t = relX / w->computedRect.w;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            w->sliderValue = w->sliderMin + t * (w->sliderMax - w->sliderMin);
            MarkDirty(static_cast<int>(i));
            if (w->onChangeCpp) w->onChangeCpp(*w);
        }
    }
}

// Handle key down event.
void UIContext::HandleKeyDown(Event& event) {
    if (event.key == KeyCode::Tab) {
        focus_.RebuildOrder(pool_, rootIndex_);
        focus_.SetCurrent(focusedWidget_);
        int next = event.mods.shift ? focus_.Prev() : focus_.Next();
        if (next >= 0) SetFocus(next, true);
        return;
    }

    // Ctrl+key shortcut
    if (event.mods.ctrl && !event.mods.alt) {
        const char* keyName = nullptr;
        switch (event.key) {
            case KeyCode::A: keyName = "Ctrl+A"; break;
            case KeyCode::B: keyName = "Ctrl+B"; break;
            case KeyCode::C: keyName = "Ctrl+C"; break;
            case KeyCode::D: keyName = "Ctrl+D"; break;
            case KeyCode::N: keyName = "Ctrl+N"; break;
            case KeyCode::O: keyName = "Ctrl+O"; break;
            case KeyCode::S: keyName = "Ctrl+S"; break;
            case KeyCode::Z: keyName = "Ctrl+Z"; break;
            case KeyCode::Y: keyName = "Ctrl+Y"; break;
            case KeyCode::Comma: keyName = "Ctrl+,"; break;
            default: break;
        }
        if (keyName) {
            StringId shortcutId = strings_.Intern(keyName);
            for (int i = 0; i < pool_.Capacity(); ++i) {
                if (!pool_.IsAlive(i)) continue;
                Widget* w = pool_.Get(i);
                if (w && w->tag == WidgetTag::MenuItem &&
                    w->shortcutTextId == shortcutId) {
                    w->flags.selected = 1;
                    theme_.NotifyStateChange(*w, i);
                    MarkDirty(i);
                    if (w->onClickId != NullStringId && scriptCallback_) {
                        scriptCallback_(strings_.Lookup(w->onClickId));
                    }
                    return;
                }
            }
        }
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

    // TextField keyboard handling
    if (focused->tag == WidgetTag::TextField && focused->flags.enabled) {
        const char* current = focused->textId != NullStringId
            ? strings_.Lookup(focused->textId) : "";
        std::string text(current);
        int len = static_cast<int>(text.size());
        if (focused->cursorPos < 0) focused->cursorPos = 0;
        if (focused->cursorPos > len) focused->cursorPos = len;

        bool changed = false;
        if (event.key == KeyCode::Backspace && focused->cursorPos > 0) {
            text.erase(focused->cursorPos - 1, 1);
            focused->cursorPos--;
            changed = true;
        } else if (event.key == KeyCode::Delete && focused->cursorPos < len) {
            text.erase(focused->cursorPos, 1);
            changed = true;
        } else if (event.key == KeyCode::Left && focused->cursorPos > 0) {
            focused->cursorPos--;
        } else if (event.key == KeyCode::Right && focused->cursorPos < len) {
            focused->cursorPos++;
        } else if (event.key == KeyCode::Home) {
            focused->cursorPos = 0;
        } else if (event.key == KeyCode::End) {
            focused->cursorPos = len;
        }

        if (changed) {
            focused->textId = strings_.Intern(text.c_str());
            MarkDirty(focusedWidget_);
            if (focused->onChangeCpp) focused->onChangeCpp(*focused);
            if (focused->onChangeId != NullStringId && scriptCallback_) {
                const char* fn = strings_.Lookup(focused->onChangeId);
                if (fn && fn[0] != '\0') scriptCallback_(fn);
            }
        }

        // Apply hex input on Enter if this is the color picker's hex field.
        if (event.key == KeyCode::Return && colorPicker_ && colorPicker_->IsOpen()) {
            colorPicker_->ApplyHexInput();
            MarkDirty(-1);
        }
    }
}

// Handle text input event.
void UIContext::HandleTextInput(Event& event) {
    Widget* focused = pool_.Get(focusedWidget_);
    if (!focused || focused->tag != WidgetTag::TextField) return;
    if (!focused->flags.enabled) return;

    const char* input = event.textInput;
    if (!input || input[0] == '\0') return;

    const char* current = focused->textId != NullStringId
        ? strings_.Lookup(focused->textId) : "";
    std::string text(current);

    int len = static_cast<int>(text.size());
    if (focused->cursorPos < 0) focused->cursorPos = 0;
    if (focused->cursorPos > len) focused->cursorPos = len;

    std::string inputStr(input);
    text.insert(focused->cursorPos, inputStr);
    focused->cursorPos += static_cast<int>(inputStr.size());

    if (text.size() > StringTable::MAX_LEN) text.resize(StringTable::MAX_LEN);

    focused->textId = strings_.Intern(text.c_str());
    MarkDirty(focusedWidget_);

    // Fire onChange callback for live text input.
    if (focused->onChangeCpp) focused->onChangeCpp(*focused);
    if (focused->onChangeId != NullStringId && scriptCallback_) {
        const char* fn = strings_.Lookup(focused->onChangeId);
        if (fn && fn[0] != '\0') scriptCallback_(fn);
    }
}

// Handle scroll event.
void UIContext::HandleScroll(Event& event) {
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

    // Walk up to find nearest scrollable element
    while (hit >= 0) {
        Widget* w = pool_.Get(hit);
        if (!w) break;
        if (w->tag == WidgetTag::ScrollView || w->flags.scrollable) {
            float delta = event.scrollDelta * 20.0f;
            if (w->scrollSmooth) {
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
            if (w->tag == WidgetTag::VirtualList) {
                UpdateVirtualList(hit);
            }
            MarkDirty(hit);
            return;
        }
        hit = w->parent;
    }
}

// Handle right-click event.
void UIContext::HandleRightClick(Event& event) {
    DismissPopup();

    int hit = HitTest(pool_, rootIndex_, event.pointerX, event.pointerY);
    if (hit < 0) return;

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

// ============================================================================
// Tree Navigation (private)
// ============================================================================

// Update visibility of tree node children after expand/collapse.
void UIContext::UpdateTreeVisibility(const Widget& toggledNode) {
    int containerIdx = toggledNode.parent;
    Widget* container = pool_.Get(containerIdx);
    if (!container) return;

    bool collapsed[32] = {};
    for (int c = 0; c < container->childCount; ++c) {
        Widget* w = pool_.Get(container->children[c]);
        if (!w || w->tag != WidgetTag::TreeNode) continue;

        int d = w->treeDepth;
        bool hidden = false;
        for (int ad = 0; ad < d && ad < 32; ++ad) {
            if (collapsed[ad]) { hidden = true; break; }
        }

        if (d > 0) {
            w->flags.visible = hidden ? 0 : 1;
        }

        if (d < 32) {
            collapsed[d] = (w->treeHasChildren && !w->expanded);
            for (int cd = d + 1; cd < 32; ++cd) collapsed[cd] = false;
        }
    }
}

// Collect visible tree node indices in display order.
std::vector<int> UIContext::GetVisibleTreeNodes(int containerIdx) const {
    std::vector<int> result;
    const Widget* container = pool_.Get(containerIdx);
    if (!container) return result;

    bool collapsed[32] = {};
    for (int c = 0; c < container->childCount; ++c) {
        const Widget* w = pool_.Get(container->children[c]);
        if (!w || w->tag != WidgetTag::TreeNode) continue;
        if (!w->flags.visible) continue;

        int d = w->treeDepth;
        bool hidden = false;
        for (int ad = 0; ad < d && ad < 32; ++ad) {
            if (collapsed[ad]) { hidden = true; break; }
        }
        if (hidden) continue;

        result.push_back(container->children[c]);

        if (d < 32) {
            collapsed[d] = (w->treeHasChildren && !w->expanded);
            for (int cd = d + 1; cd < 32; ++cd) collapsed[cd] = false;
        }
    }
    return result;
}

// Scroll a scrollview so that a child widget is visible.
void UIContext::ScrollNodeIntoView(int nodeIdx) {
    Widget* w = pool_.Get(nodeIdx);
    if (!w) return;
    int parentIdx = w->parent;
    Widget* parent = pool_.Get(parentIdx);
    if (!parent) return;
    while (parent && parent->tag != WidgetTag::ScrollView && !parent->flags.scrollable) {
        parentIdx = parent->parent;
        parent = pool_.Get(parentIdx);
    }
    if (!parent) return;

    Rect pr = parent->computedRect;
    Rect wr = w->computedRect;

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

// Handle arrow key navigation within a tree node structure.
void UIContext::HandleTreeNavigation(KeyCode key, Modifiers mods) {
    Widget* focused = pool_.Get(focusedWidget_);
    if (!focused || focused->tag != WidgetTag::TreeNode) return;

    int containerIdx = focused->parent;
    if (containerIdx < 0) return;

    if (key == KeyCode::Left) {
        if (focused->treeHasChildren && focused->expanded) {
            focused->expanded = false;
            MarkDirty(-1);
        } else if (focused->treeDepth > 0) {
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

    // Up / Down / Home / End
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

// ============================================================================
// Click Dispatch (private)
// ============================================================================

// Deselect radio buttons in the same group recursively.
void UIContext::DeselectRadioGroupRecursive(Widget* parent, StringId group, Widget* except) {
    for (int i = 0; i < parent->childCount; ++i) {
        Widget* child = pool_.Get(parent->children[i]);
        if (!child) continue;
        if (child->tag == WidgetTag::RadioButton &&
            child->radioGroup == group && child != except) {
            child->checked = false;
        }
        if (child->childCount > 0) {
            DeselectRadioGroupRecursive(child, group, except);
        }
    }
}

// Dispatch a click on a widget.
void UIContext::DispatchClick(Widget& w) {
    bool handled = false;
    switch (w.tag) {
        case WidgetTag::Checkbox:
            w.checked = !w.checked;
            MarkDirty(-1);
            if (w.onChangeCpp) w.onChangeCpp(w);
            if (w.onChangeId != NullStringId && scriptCallback_) {
                const char* fn = strings_.Lookup(w.onChangeId);
                if (fn && fn[0] != '\0') scriptCallback_(fn);
            }
            handled = true;
            break;
        case WidgetTag::Dropdown:
            if (w.dropdownOpen) {
                float itemH = w.computedRect.h;
                float popupY = w.computedRect.y + w.computedRect.h + 2.0f;
                float popupH = w.childCount * itemH;
                if (lastPointerY_ >= popupY && lastPointerY_ < popupY + popupH) {
                    int idx = static_cast<int>((lastPointerY_ - popupY) / itemH);
                    if (idx >= 0 && idx < w.childCount) {
                        w.selectedIndex = idx;
                        Widget* sel = pool_.Get(w.children[idx]);
                        if (sel && sel->textId != NullStringId) {
                            w.textId = sel->textId;
                        }
                        if (w.onChangeCpp) w.onChangeCpp(w);
                    }
                }
                w.dropdownOpen = false;
            } else {
                w.dropdownOpen = true;
            }
            MarkDirty(-1);
            handled = true;
            break;
        case WidgetTag::TreeNode: {
            float indent = w.treeDepth * 16.0f;
            float boxSize = 10.0f;
            Rect cr = w.computedRect;
            float boxLeft = cr.x + w.padding.left + indent;
            float boxRight = boxLeft + boxSize;

            bool clickedBox = w.treeHasChildren &&
                lastPointerX_ >= boxLeft && lastPointerX_ <= boxRight;

            if (clickedBox) {
                w.expanded = !w.expanded;
                UpdateTreeVisibility(w);
            } else {
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
            MarkDirty(-1);
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
                if (w.radioGroup != NullStringId) {
                    int searchRoot = w.parent;
                    Widget* sp = pool_.Get(searchRoot);
                    while (sp && sp->parent >= 0) {
                        searchRoot = sp->parent;
                        sp = pool_.Get(searchRoot);
                        if (!sp) break;
                        if (sp->childCount > 1) break;
                    }
                    if (sp) {
                        DeselectRadioGroupRecursive(sp, w.radioGroup, &w);
                    }
                }
                MarkDirty(-1);
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
            MarkDirty(-1);
            if (w.onChangeCpp) w.onChangeCpp(w);
            if (w.onChangeId != NullStringId && scriptCallback_) {
                const char* fn = strings_.Lookup(w.onChangeId);
                if (fn && fn[0] != '\0') scriptCallback_(fn);
            }
            handled = true;
            break;
        }
        case WidgetTag::ColorField: {
            ColorPicker& picker = EnsureColorPicker();
            int widgetIdx = -1;
            for (size_t si = 0; si < pool_.Capacity(); ++si) {
                if (pool_.Get(static_cast<int>(si)) == &w) {
                    widgetIdx = static_cast<int>(si);
                    break;
                }
            }
            if (widgetIdx >= 0) {
                if (picker.IsOpen()) picker.Close();
                float px = w.computedRect.x;
                float py = w.computedRect.y + w.computedRect.h + 4.0f;
                picker.Open(widgetIdx, px, py);
                MarkDirty(-1);
            }
            handled = true;
            break;
        }
        case WidgetTag::MenuItem:
            if (w.submenuIdx >= 0) {
                int itemIdx = -1;
                for (size_t si = 0; si < pool_.Capacity(); ++si) {
                    if (pool_.Get(static_cast<int>(si)) == &w) { itemIdx = static_cast<int>(si); break; }
                }
                if (itemIdx >= 0) ShowSubMenu(itemIdx, w.submenuIdx);
                handled = true;
            } else {
                DismissPopup();
                handled = false;
            }
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

    // Event bubbling
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

// Check if a widget can accept the given drag payload.
bool UIContext::CanAcceptDrop(const Widget& target, const DragPayload& payload) const {
    if (payload.typeTag == DragType::None) return false;
    if (target.acceptDropTypes != 0) {
        if (payload.typeTag < 32 && !(target.acceptDropTypes & (1u << payload.typeTag)))
            return false;
    } else if (!target.onCanDrop) {
        return false;
    }
    if (target.onCanDrop) return target.onCanDrop(payload);
    return target.acceptDropTypes != 0;
}

// ============================================================================
// Floating Panel Handling (private)
// ============================================================================

// Check if (px,py) hits a floating panel's title bar or resize edge.
bool UIContext::FindFloatingPanelHit(float px, float py) {
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
        float closeBtnX = r.x + r.w - btnSize - 4.0f;
        float btnY = r.y + 3.0f;
        if (px >= closeBtnX && px <= closeBtnX + btnSize && py >= btnY && py <= btnY + btnSize) {
            SetVisible(bestIdx, false);
            return true;
        }
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

// Handle floating panel drag movement.
void UIContext::HandlePanelDrag(float px, float py) {
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

// Returns the appropriate cursor type if the mouse is over a floating panel edge.
CursorType UIContext::GetFloatingPanelEdgeCursor(float px, float py) const {
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

// Minimize a floating panel to the status bar tray.
void UIContext::MinimizePanel(int panelIdx) {
    Widget* w = pool_.Get(panelIdx);
    if (!w || w->tag != WidgetTag::FloatingPanel) return;
    w->flags.minimized = 1;
    w->flags.visible = 0;
    MarkDirty(panelIdx);
}

// Restore a minimized floating panel.
void UIContext::RestorePanel(int panelIdx) {
    Widget* w = pool_.Get(panelIdx);
    if (!w || w->tag != WidgetTag::FloatingPanel) return;
    w->flags.minimized = 0;
    w->flags.visible = 1;
    BringToFront(panelIdx);
    MarkDirty(panelIdx);
}

// Get all minimized floating panels.
std::vector<int> UIContext::GetMinimizedPanels() const {
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

// Check if click hit a minimized panel tray button.
bool UIContext::FindMinimizedTrayHit(float px, float py) {
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

// ============================================================================
// Divider Resize (private)
// ============================================================================

// Check if (px,py) is on a divider between children of a layout container.
bool UIContext::FindDividerHit(float px, float py) {
    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        Widget* parent = pool_.Get(static_cast<int>(i));
        if (!parent || !pool_.IsAlive(static_cast<int>(i))) continue;
        if (!parent->flags.visible || parent->childCount < 2) continue;
        if (!parent->flags.resizable) continue;

        bool isRow = (parent->layout.direction == LayoutDirection::Row);

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
                    float top = std::min(childA->computedRect.y, childB->computedRect.y);
                    float bot = std::max(childA->computedRect.y + childA->computedRect.h,
                                         childB->computedRect.y + childB->computedRect.h);
                    if (py < top || py > bot) { prevChild = parent->children[c]; continue; }
                } else {
                    dividerPos = childA->computedRect.y + childA->computedRect.h;
                    mousePos = py;
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

// Handle resize drag movement.
void UIContext::HandleResizeDrag(float px, float py) {
    if (!resizeDrag_.active) return;

    Widget* childA = pool_.Get(resizeDrag_.childA);
    Widget* childB = pool_.Get(resizeDrag_.childB);
    if (!childA || !childB) { resizeDrag_.active = false; return; }

    float mousePos = resizeDrag_.horizontal ? px : py;
    float delta = mousePos - resizeDrag_.startMouse;

    float newSizeA = resizeDrag_.startSizeA + delta;
    float newSizeB = resizeDrag_.startSizeB - delta;

    if (newSizeA < MIN_PANEL_SIZE) {
        newSizeA = MIN_PANEL_SIZE;
        newSizeB = resizeDrag_.startSizeA + resizeDrag_.startSizeB - MIN_PANEL_SIZE;
    }
    if (newSizeB < MIN_PANEL_SIZE) {
        newSizeB = MIN_PANEL_SIZE;
        newSizeA = resizeDrag_.startSizeA + resizeDrag_.startSizeB - MIN_PANEL_SIZE;
    }

    SizeMode modeA = resizeDrag_.horizontal ? childA->widthMode : childA->heightMode;
    SizeMode modeB = resizeDrag_.horizontal ? childB->widthMode : childB->heightMode;

    if (modeA == SizeMode::FillRemaining && modeB == SizeMode::FillRemaining) {
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
        if (resizeDrag_.horizontal)
            childB->localW = newSizeB;
        else
            childB->localH = newSizeB;
    } else if (modeB == SizeMode::FillRemaining) {
        if (resizeDrag_.horizontal)
            childA->localW = newSizeA;
        else
            childA->localH = newSizeA;
    } else {
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

// ============================================================================
// Scrollbar Handling (private)
// ============================================================================

// Check if (px,py) hits a scrollbar track/thumb on any ScrollView.
bool UIContext::FindScrollbarHit(float px, float py) {
    constexpr float BAR_WIDTH = 8.0f;
    for (size_t i = 0; i < pool_.Capacity(); ++i) {
        Widget* w = pool_.Get(static_cast<int>(i));
        if (!w || !pool_.IsAlive(static_cast<int>(i))) continue;
        if (w->tag != WidgetTag::ScrollView && !w->flags.scrollable) continue;
        if (!w->flags.visible) continue;
        if (w->contentHeight <= w->computedRect.h) continue;

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

// Handle scrollbar drag movement.
void UIContext::HandleScrollDrag(float py) {
    if (!scrollDrag_.active) return;
    Widget* sv = pool_.Get(scrollDrag_.scrollViewIdx);
    if (!sv) { scrollDrag_.active = false; return; }

    float trackH = sv->computedRect.h;
    float contentH = sv->contentHeight;
    float maxScroll = contentH - trackH;
    if (maxScroll <= 0.0f) { scrollDrag_.active = false; return; }

    float mouseDelta = py - scrollDrag_.startMouseY;
    float scrollDelta = mouseDelta * (contentH / trackH);
    sv->scrollY = scrollDrag_.startScrollY - scrollDelta;

    if (sv->scrollY > 0.0f) sv->scrollY = 0.0f;
    if (sv->scrollY < -maxScroll) sv->scrollY = -maxScroll;
    MarkDirty(scrollDrag_.scrollViewIdx);
}

// ============================================================================
// Color Picker Integration
// ============================================================================

// Lazy-build the shared color picker popup on first use.
ColorPicker& UIContext::EnsureColorPicker() {
    if (!colorPicker_) {
        colorPicker_ = std::make_unique<ColorPicker>();
        colorPicker_->Build(*this);
    }
    return *colorPicker_;
}

} // namespace ui
} // namespace koilo
