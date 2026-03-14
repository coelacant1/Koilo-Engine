// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file event.hpp
 * @brief Event types, propagation, and hit testing for the UI engine.
 *
 * Events bubble from target widget up to root. Handlers can stop
 * propagation. Hit testing walks the tree front-to-back.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "widget.hpp"
#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

// --- Event Types ---

enum class EventType : uint8_t {
    None = 0,
    PointerDown,   ///< Pointer button pressed.
    PointerUp,     ///< Pointer button released.
    PointerMove,   ///< Pointer moved.
    PointerEnter,  ///< Pointer entered widget bounds.
    PointerExit,   ///< Pointer left widget bounds.
    Click,         ///< Single click.
    DoubleClick,   ///< Double click.
    KeyDown,       ///< Key pressed.
    KeyUp,         ///< Key released.
    TextInput,     ///< Text character input.
    Scroll,        ///< Scroll wheel event.
    FocusGain,     ///< Widget gained focus.
    FocusLost,     ///< Widget lost focus.
    ValueChanged,  ///< Widget value changed.
    RightClick,    ///< Right-click / context menu.
    DragStart,     ///< Drag operation started.
    DragMove,      ///< Drag in progress.
    DragEnd,       ///< Drag operation ended.
    Drop           ///< Payload dropped on widget.
};

// --- Key Codes ---

enum class KeyCode : uint16_t {
    None = 0,
    Tab, Escape, Return, Backspace, Delete,
    Left, Right, Up, Down,
    Home, End, PageUp, PageDown,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5,
    Num6, Num7, Num8, Num9,
    Space, Minus, Plus, Period, Comma,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

// --- Modifier Flags ---

/**
 * @class Modifiers
 * @brief Keyboard modifier key state flags (shift, ctrl, alt, super).
 */
struct Modifiers {
    uint8_t shift : 1; ///< Shift key is held.
    uint8_t ctrl  : 1; ///< Ctrl key is held.
    uint8_t alt   : 1; ///< Alt key is held.
    uint8_t super : 1; ///< Super/Meta key is held.
    uint8_t pad   : 4; ///< Padding bits.

    KL_BEGIN_FIELDS(Modifiers)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Modifiers)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Modifiers)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Modifiers)

};

// --- Event ---

/**
 * @class Event
 * @brief UI event carrying pointer, key, text, and drag-and-drop data.
 *
 * Events bubble from the target widget up to the root. Handlers can call
 * Consume() to stop propagation.
 */
struct Event {
    EventType type = EventType::None; ///< Type of this event.
    int target = -1;                  ///< Widget pool index of the target.

    // -- Pointer data ---------------------------------------------
    float pointerX = 0.0f;       ///< Pointer X coordinate.
    float pointerY = 0.0f;       ///< Pointer Y coordinate.
    float scrollDelta = 0.0f;    ///< Scroll wheel delta.
    uint8_t pointerButton = 0;   ///< Button index (0=left, 1=right, 2=middle).

    // -- Key data -------------------------------------------------
    KeyCode key = KeyCode::None; ///< Key code for key events.
    Modifiers mods{};            ///< Modifier key state.

    // -- Text input (UTF-8, up to 7 bytes + null) -----------------
    char textInput[8]{};         ///< UTF-8 text input buffer.

    // -- Drag-and-drop payload ------------------------------------
    int dragSource = -1;              ///< Widget pool index of drag origin.
    const void* dragData = nullptr;   ///< Opaque user data pointer.
    uint32_t dragTag = 0;             ///< User-defined payload type tag.

    // -- Propagation control --------------------------------------
    bool consumed = false; ///< True if the event has been consumed.

    /** @brief Mark this event as consumed, stopping propagation. */
    void Consume() { consumed = true; }

    KL_BEGIN_FIELDS(Event)
        KL_FIELD(Event, target, "Target", 0, 0),
        KL_FIELD(Event, pointerX, "Pointer X", 0, 0),
        KL_FIELD(Event, pointerY, "Pointer Y", 0, 0),
        KL_FIELD(Event, consumed, "Consumed", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Event)
        KL_METHOD_AUTO(Event, Consume, "Consume")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Event)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Event)

};

// --- Event Queue ---

/**
 * @class EventQueue
 * @brief Fixed-capacity ring-less event queue for buffering UI events per frame.
 */
class EventQueue {
public:
    static constexpr size_t CAPACITY = 256; ///< Maximum events per frame.

    /**
     * @brief Push an event onto the queue.
     * @param e The event to enqueue.
     */
    void Push(const Event& e) {
        if (count_ < CAPACITY) {
            events_[count_++] = e;
        }
    }

    /**
     * @brief Access an event by index.
     * @param i Index of the event (0-based).
     * @return Const reference to the event.
     */
    const Event& At(size_t i) const { return events_[i]; }
    /** @brief Get the number of queued events. */
    size_t Count() const { return count_; }
    /** @brief Clear all queued events. */
    void Clear() { count_ = 0; }

    /** @brief Get iterator to the first event. */
    const Event* begin() const { return events_; }
    /** @brief Get iterator past the last event. */
    const Event* end() const { return events_ + count_; }

private:
    Event events_[CAPACITY]{}; ///< Event storage array.
    size_t count_ = 0;         ///< Current number of queued events.
};

// --- Hit Testing ---

/**
 * @brief Walk widget tree front-to-back, returning the deepest hit widget.
 * @param pool The widget pool to search.
 * @param rootIndex Pool index of the root widget to start from.
 * @param px X coordinate of the point to test.
 * @param py Y coordinate of the point to test.
 * @return Pool index of the deepest widget containing the point, or -1 if no hit.
 */
inline int HitTest(const WidgetPool& pool, int rootIndex, float px, float py) {
    const Widget* root = pool.Get(rootIndex);
    if (!root) return -1;
    if (!root->flags.visible) return -1;

    // Expand hit rect for open dropdowns to include popup area
    Rect hitRect = root->computedRect;
    if (root->tag == WidgetTag::Dropdown && root->dropdownOpen && root->childCount > 0) {
        hitRect.h += 2.0f + root->childCount * root->computedRect.h;
    }
    if (!hitRect.Contains(px, py)) return -1;

    // Sort children by z-order (highest first) for correct hit priority
    int16_t sorted[MAX_CHILDREN];
    int16_t count = root->childCount;
    for (int16_t i = 0; i < count; ++i) sorted[i] = root->children[i];
    // Reverse z-order sort (highest z first = front-most panel tested first)
    for (int16_t i = 1; i < count; ++i) {
        int16_t key = sorted[i];
        const Widget* kw = pool.Get(key);
        int16_t kz = kw ? kw->zOrder : 0;
        int16_t j = i - 1;
        while (j >= 0) {
            const Widget* jw = pool.Get(sorted[j]);
            int16_t jz = jw ? jw->zOrder : 0;
            if (jz >= kz) break;
            sorted[j + 1] = sorted[j];
            --j;
        }
        sorted[j + 1] = key;
    }

    for (int16_t i = 0; i < count; ++i) {
        int childIdx = sorted[i];
        int hit = HitTest(pool, childIdx, px, py);
        if (hit >= 0) return hit;
    }

    // pointer-events: none - transparent to clicks, pass through to parent
    if (!root->flags.pointerEvents) return -1;

    // No child hit, return self if enabled
    if (root->flags.enabled) return rootIndex;
    return -1;
}

// --- Focus Management ---

/**
 * @class FocusManager
 * @brief Manages keyboard focus traversal order across focusable widgets.
 */
class FocusManager {
public:
    /**
     * @brief Rebuild the focus traversal order from the widget tree.
     * @param pool The widget pool containing all widgets.
     * @param rootIndex Pool index of the root widget.
     */
    void RebuildOrder(const WidgetPool& pool, int rootIndex) {
        count_ = 0;
        CollectFocusable(pool, rootIndex);
        currentIndex_ = -1;
    }

    /** @brief Get the pool index of the currently focused widget. */
    int Current() const {
        if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(count_)) return -1;
        return order_[currentIndex_];
    }

    /** @brief Advance focus to the next focusable widget and return its pool index. */
    int Next() {
        if (count_ == 0) return -1;
        currentIndex_ = (currentIndex_ + 1) % static_cast<int>(count_);
        return order_[currentIndex_];
    }

    /** @brief Move focus to the previous focusable widget and return its pool index. */
    int Prev() {
        if (count_ == 0) return -1;
        currentIndex_ = (currentIndex_ - 1 + static_cast<int>(count_)) % static_cast<int>(count_);
        return order_[currentIndex_];
    }

    /**
     * @brief Set focus to a specific widget by pool index.
     * @param widgetIndex Pool index of the widget to focus.
     */
    void SetCurrent(int widgetIndex) {
        for (size_t i = 0; i < count_; ++i) {
            if (order_[i] == widgetIndex) {
                currentIndex_ = static_cast<int>(i);
                return;
            }
        }
    }

    /** @brief Get the number of focusable widgets. */
    size_t Count() const { return count_; }

private:
    static constexpr size_t MAX_FOCUS = 512; ///< Maximum number of focusable widgets.

    void CollectFocusable(const WidgetPool& pool, int index) {
        const Widget* w = pool.Get(index);
        if (!w || !w->flags.visible || !w->flags.enabled) return;
        if (w->flags.focusable && count_ < MAX_FOCUS) {
            order_[count_++] = index;
        }
        for (int i = 0; i < w->childCount; ++i) {
            CollectFocusable(pool, w->children[i]);
        }
    }

    int order_[MAX_FOCUS]{};   ///< Focusable widget pool indices in traversal order.
    size_t count_ = 0;        ///< Number of focusable widgets.
    int currentIndex_ = -1;   ///< Current position in the focus order (-1 = none).
};

} // namespace ui
} // namespace koilo
