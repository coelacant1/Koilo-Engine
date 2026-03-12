// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file event.hpp
 * @brief Event types, propagation, and hit testing for the UI engine.
 *
 * Events bubble from target widget up to root. Handlers can stop
 * propagation. Hit testing walks the tree front-to-back.
 *
 * @date 03/08/2026
 * @author Coela
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
    PointerDown,
    PointerUp,
    PointerMove,
    PointerEnter,
    PointerExit,
    Click,
    DoubleClick,
    KeyDown,
    KeyUp,
    TextInput,
    Scroll,
    FocusGain,
    FocusLost,
    ValueChanged,
    RightClick,
    DragStart,
    DragMove,
    DragEnd,
    Drop
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

struct Modifiers {
    uint8_t shift : 1;
    uint8_t ctrl  : 1;
    uint8_t alt   : 1;
    uint8_t super : 1;
    uint8_t pad   : 4;

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

struct Event {
    EventType type = EventType::None;
    int target = -1;     // widget pool index

    // Pointer data
    float pointerX = 0.0f;
    float pointerY = 0.0f;
    float scrollDelta = 0.0f;
    uint8_t pointerButton = 0;  // 0=left, 1=right, 2=middle

    // Key data
    KeyCode key = KeyCode::None;
    Modifiers mods{};

    // Text input (UTF-8, up to 7 bytes + null)
    char textInput[8]{};

    // Drag-and-drop payload
    int dragSource = -1;   // widget pool index of drag origin
    const void* dragData = nullptr;  // opaque user data pointer
    uint32_t dragTag = 0;  // user-defined payload type tag

    // Propagation control
    bool consumed = false;

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

class EventQueue {
public:
    static constexpr size_t CAPACITY = 256;

    void Push(const Event& e) {
        if (count_ < CAPACITY) {
            events_[count_++] = e;
        }
    }

    const Event& At(size_t i) const { return events_[i]; }
    size_t Count() const { return count_; }
    void Clear() { count_ = 0; }

    const Event* begin() const { return events_; }
    const Event* end() const { return events_ + count_; }

private:
    Event events_[CAPACITY]{};
    size_t count_ = 0;
};

// --- Hit Testing ---

/// Walk widget tree front-to-back, return pool index of deepest widget
/// containing the point. Returns -1 if no hit.
inline int HitTest(const WidgetPool& pool, int rootIndex, float px, float py) {
    const Widget* root = pool.Get(rootIndex);
    if (!root) return -1;
    if (!root->flags.visible) return -1;
    if (!root->computedRect.Contains(px, py)) return -1;

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

class FocusManager {
public:
    void RebuildOrder(const WidgetPool& pool, int rootIndex) {
        count_ = 0;
        CollectFocusable(pool, rootIndex);
        currentIndex_ = -1;
    }

    int Current() const {
        if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(count_)) return -1;
        return order_[currentIndex_];
    }

    int Next() {
        if (count_ == 0) return -1;
        currentIndex_ = (currentIndex_ + 1) % static_cast<int>(count_);
        return order_[currentIndex_];
    }

    int Prev() {
        if (count_ == 0) return -1;
        currentIndex_ = (currentIndex_ - 1 + static_cast<int>(count_)) % static_cast<int>(count_);
        return order_[currentIndex_];
    }

    void SetCurrent(int widgetIndex) {
        for (size_t i = 0; i < count_; ++i) {
            if (order_[i] == widgetIndex) {
                currentIndex_ = static_cast<int>(i);
                return;
            }
        }
    }

    size_t Count() const { return count_; }

private:
    static constexpr size_t MAX_FOCUS = 512;

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

    int order_[MAX_FOCUS]{};
    size_t count_ = 0;
    int currentIndex_ = -1;
};

} // namespace ui
} // namespace koilo
