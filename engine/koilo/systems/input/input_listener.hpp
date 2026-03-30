// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file input_listener.hpp
 * @brief Event-based input with priority dispatch and consumption.
 *
 * Provides event structs for keyboard, mouse, and scroll input,
 * an IInputListener interface for priority-ordered consumption,
 * and InputListenerRegistry for managing listeners.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include "keycodes.hpp"
#include <koilo/core/math/vector2d.hpp>

namespace koilo {

// -- Event structs -----------------------------------------------

enum class KeyAction : uint8_t { Pressed, Released };
enum class MouseButtonAction : uint8_t { Pressed, Released };

struct KeyEvent {
    KeyCode   key    = KeyCode::Unknown;
    KeyAction action = KeyAction::Pressed;
    bool shift = false;
    bool ctrl  = false;
    bool alt   = false;
    bool consumed = false;
};

struct MouseButtonEvent {
    MouseButton       button = MouseButton::Left;
    MouseButtonAction action = MouseButtonAction::Pressed;
    Vector2D          position;
    bool consumed = false;
};

struct MouseMoveEvent {
    Vector2D position;
    Vector2D delta;
    bool consumed = false;
};

struct ScrollEvent {
    Vector2D delta;
    bool consumed = false;
};

// -- Listener interface ------------------------------------------

/**
 * @class IInputListener
 * @brief Interface for priority-based input event consumption.
 *
 * Return true from any handler to consume the event and stop
 * propagation to lower-priority listeners.
 */
class IInputListener {
public:
    virtual ~IInputListener() = default;

    virtual const char* GetName() const = 0;

    /// Higher priority listeners receive events first.
    virtual int GetPriority() const = 0;

    virtual bool OnKeyEvent(KeyEvent& event) { (void)event; return false; }
    virtual bool OnMouseButtonEvent(MouseButtonEvent& event) { (void)event; return false; }
    virtual bool OnMouseMoveEvent(MouseMoveEvent& event) { (void)event; return false; }
    virtual bool OnScrollEvent(ScrollEvent& event) { (void)event; return false; }
};

// -- Listener registry -------------------------------------------

/**
 * @class InputListenerRegistry
 * @brief Manages a priority-sorted set of input listeners.
 *
 * Listeners are dispatched highest-priority first. If a listener
 * returns true the event is marked consumed and propagation stops.
 */
class InputListenerRegistry {
public:
    void Register(IInputListener* listener) {
        if (!listener) return;
        listeners_.push_back(listener);
        dirty_ = true;
    }

    void Unregister(IInputListener* listener) {
        auto it = std::find(listeners_.begin(), listeners_.end(), listener);
        if (it != listeners_.end()) {
            listeners_.erase(it);
        }
    }

    void DispatchKey(KeyEvent& event) {
        EnsureSorted();
        for (auto* l : listeners_) {
            if (l->OnKeyEvent(event)) {
                event.consumed = true;
                return;
            }
        }
    }

    void DispatchMouseButton(MouseButtonEvent& event) {
        EnsureSorted();
        for (auto* l : listeners_) {
            if (l->OnMouseButtonEvent(event)) {
                event.consumed = true;
                return;
            }
        }
    }

    void DispatchMouseMove(MouseMoveEvent& event) {
        EnsureSorted();
        for (auto* l : listeners_) {
            if (l->OnMouseMoveEvent(event)) {
                event.consumed = true;
                return;
            }
        }
    }

    void DispatchScroll(ScrollEvent& event) {
        EnsureSorted();
        for (auto* l : listeners_) {
            if (l->OnScrollEvent(event)) {
                event.consumed = true;
                return;
            }
        }
    }

    size_t Count() const { return listeners_.size(); }

    const std::vector<IInputListener*>& GetListeners() const { return listeners_; }

private:
    void EnsureSorted() {
        if (!dirty_) return;
        std::stable_sort(listeners_.begin(), listeners_.end(),
            [](const IInputListener* a, const IInputListener* b) {
                return a->GetPriority() > b->GetPriority();
            });
        dirty_ = false;
    }

    std::vector<IInputListener*> listeners_;
    bool dirty_ = false;
};

} // namespace koilo
