// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/input/inputmanager.hpp>

namespace koilo {

koilo::InputManager::InputManager() {
    // Initialize gamepads
    for (int i = 0; i < MaxGamepads; ++i) {
        gamepads[i] = Gamepad(i);
    }
}

void koilo::InputManager::Update() {
    keyboard.Update();
    mouse.Update();

    for (auto& pair : gamepads) {
        pair.second.Update();
    }

    GenerateAndDispatchEvents();
}

Gamepad& koilo::InputManager::GetGamepad(int id) {
    // Clamp ID to valid range
    if (id < 0) id = 0;
    if (id >= MaxGamepads) id = MaxGamepads - 1;

    // Create gamepad if it doesn't exist
    if (gamepads.find(id) == gamepads.end()) {
        gamepads[id] = Gamepad(id);
    }

    return gamepads[id];
}

bool koilo::InputManager::IsGamepadConnected(int id) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return false;
    return it->second.IsConnected();
}

bool koilo::InputManager::IsGamepadButtonPressed(int id, GamepadButton button) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return false;
    return it->second.IsButtonPressed(button);
}

bool koilo::InputManager::IsGamepadButtonHeld(int id, GamepadButton button) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return false;
    return it->second.IsButtonHeld(button);
}

float koilo::InputManager::GetGamepadAxis(int id, GamepadAxis axis) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return 0.0f;
    return it->second.GetAxisValue(axis);
}

// Action mapping implementations

void koilo::InputManager::MapAction(const std::string& action, KeyCode key) {
    actionToKey[action] = key;
}

void koilo::InputManager::MapAction(const std::string& action, MouseButton button) {
    actionToMouseButton[action] = button;
}

void koilo::InputManager::MapAction(const std::string& action, GamepadButton button) {
    actionToGamepadButton[action] = button;
}

bool koilo::InputManager::IsActionPressed(const std::string& action) const {
    // Check keyboard
    auto keyIt = actionToKey.find(action);
    if (keyIt != actionToKey.end()) {
        if (keyboard.IsKeyPressed(keyIt->second)) {
            return true;
        }
    }

    // Check mouse
    auto mouseIt = actionToMouseButton.find(action);
    if (mouseIt != actionToMouseButton.end()) {
        if (mouse.IsButtonPressed(mouseIt->second)) {
            return true;
        }
    }

    // Check gamepad (check first connected gamepad)
    auto gamepadIt = actionToGamepadButton.find(action);
    if (gamepadIt != actionToGamepadButton.end()) {
        for (const auto& pair : gamepads) {
            if (pair.second.IsConnected() && pair.second.IsButtonPressed(gamepadIt->second)) {
                return true;
            }
        }
    }

    return false;
}

bool koilo::InputManager::IsActionHeld(const std::string& action) const {
    // Check keyboard
    auto keyIt = actionToKey.find(action);
    if (keyIt != actionToKey.end()) {
        if (keyboard.IsKeyHeld(keyIt->second)) {
            return true;
        }
    }

    // Check mouse
    auto mouseIt = actionToMouseButton.find(action);
    if (mouseIt != actionToMouseButton.end()) {
        if (mouse.IsButtonHeld(mouseIt->second)) {
            return true;
        }
    }

    // Check gamepad
    auto gamepadIt = actionToGamepadButton.find(action);
    if (gamepadIt != actionToGamepadButton.end()) {
        for (const auto& pair : gamepads) {
            if (pair.second.IsConnected() && pair.second.IsButtonHeld(gamepadIt->second)) {
                return true;
            }
        }
    }

    return false;
}

// Axis mapping implementations

void koilo::InputManager::MapAxis(const std::string& axis, GamepadAxis gamepadAxis) {
    axisMapping[axis] = gamepadAxis;
}

float koilo::InputManager::GetAxis(const std::string& axis, int gamepadId) const {
    auto it = axisMapping.find(axis);
    if (it == axisMapping.end()) return 0.0f;

    return GetGamepadAxis(gamepadId, it->second);
}

void koilo::InputManager::GenerateAndDispatchEvents() {
    if (listenerRegistry_.Count() == 0) return;

    // Skip event generation on first frame (no previous state)
    if (firstFrame_) {
        firstFrame_ = false;
        for (int k = 0; k < static_cast<int>(KeyCode::MaxKeyCode); ++k) {
            prevKeys_[k] = keyboard.IsKeyHeld(static_cast<KeyCode>(k));
        }
        for (int b = 0; b < static_cast<int>(MouseButton::MaxButton); ++b) {
            prevMouseButtons_[b] = mouse.IsButtonHeld(static_cast<MouseButton>(b));
        }
        prevMousePos_ = mouse.GetPosition();
        return;
    }

    bool shift = keyboard.IsShiftPressed();
    bool ctrl  = keyboard.IsCtrlPressed();
    bool alt   = keyboard.IsAltPressed();

    // Keyboard events
    for (int k = 0; k < static_cast<int>(KeyCode::MaxKeyCode); ++k) {
        bool cur  = keyboard.IsKeyHeld(static_cast<KeyCode>(k));
        bool prev = prevKeys_[k];
        if (cur && !prev) {
            KeyEvent ev;
            ev.key = static_cast<KeyCode>(k);
            ev.action = KeyAction::Pressed;
            ev.shift = shift; ev.ctrl = ctrl; ev.alt = alt;
            listenerRegistry_.DispatchKey(ev);
        } else if (!cur && prev) {
            KeyEvent ev;
            ev.key = static_cast<KeyCode>(k);
            ev.action = KeyAction::Released;
            ev.shift = shift; ev.ctrl = ctrl; ev.alt = alt;
            listenerRegistry_.DispatchKey(ev);
        }
        prevKeys_[k] = cur;
    }

    // Mouse button events
    Vector2D mousePos = mouse.GetPosition();
    for (int b = 0; b < static_cast<int>(MouseButton::MaxButton); ++b) {
        bool cur  = mouse.IsButtonHeld(static_cast<MouseButton>(b));
        bool prev = prevMouseButtons_[b];
        if (cur && !prev) {
            MouseButtonEvent ev;
            ev.button = static_cast<MouseButton>(b);
            ev.action = MouseButtonAction::Pressed;
            ev.position = mousePos;
            listenerRegistry_.DispatchMouseButton(ev);
        } else if (!cur && prev) {
            MouseButtonEvent ev;
            ev.button = static_cast<MouseButton>(b);
            ev.action = MouseButtonAction::Released;
            ev.position = mousePos;
            listenerRegistry_.DispatchMouseButton(ev);
        }
        prevMouseButtons_[b] = cur;
    }

    // Mouse move event
    Vector2D mouseDelta = mouse.GetDelta();
    if (mouseDelta.X != 0.0f || mouseDelta.Y != 0.0f) {
        MouseMoveEvent ev;
        ev.position = mousePos;
        ev.delta = mouseDelta;
        listenerRegistry_.DispatchMouseMove(ev);
    }

    // Scroll event
    Vector2D scroll = mouse.GetScrollDelta();
    if (scroll.X != 0.0f || scroll.Y != 0.0f) {
        ScrollEvent ev;
        ev.delta = scroll;
        listenerRegistry_.DispatchScroll(ev);
    }

    prevMousePos_ = mousePos;
}

} // namespace koilo
