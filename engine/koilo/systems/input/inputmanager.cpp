#include <ptx/systems/input/inputmanager.hpp>

namespace ptx {

InputManager::InputManager() {
    // Initialize gamepads
    for (int i = 0; i < MaxGamepads; ++i) {
        gamepads[i] = Gamepad(i);
    }
}

void InputManager::Update() {
    keyboard.Update();
    mouse.Update();

    for (auto& pair : gamepads) {
        pair.second.Update();
    }
}

Gamepad& InputManager::GetGamepad(int id) {
    // Clamp ID to valid range
    if (id < 0) id = 0;
    if (id >= MaxGamepads) id = MaxGamepads - 1;

    // Create gamepad if it doesn't exist
    if (gamepads.find(id) == gamepads.end()) {
        gamepads[id] = Gamepad(id);
    }

    return gamepads[id];
}

bool InputManager::IsGamepadConnected(int id) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return false;
    return it->second.IsConnected();
}

bool InputManager::IsGamepadButtonPressed(int id, GamepadButton button) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return false;
    return it->second.IsButtonPressed(button);
}

bool InputManager::IsGamepadButtonHeld(int id, GamepadButton button) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return false;
    return it->second.IsButtonHeld(button);
}

float InputManager::GetGamepadAxis(int id, GamepadAxis axis) const {
    auto it = gamepads.find(id);
    if (it == gamepads.end()) return 0.0f;
    return it->second.GetAxisValue(axis);
}

// Action mapping implementations

void InputManager::MapAction(const std::string& action, KeyCode key) {
    actionToKey[action] = key;
}

void InputManager::MapAction(const std::string& action, MouseButton button) {
    actionToMouseButton[action] = button;
}

void InputManager::MapAction(const std::string& action, GamepadButton button) {
    actionToGamepadButton[action] = button;
}

bool InputManager::IsActionPressed(const std::string& action) const {
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

bool InputManager::IsActionHeld(const std::string& action) const {
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

void InputManager::MapAxis(const std::string& axis, GamepadAxis gamepadAxis) {
    axisMapping[axis] = gamepadAxis;
}

float InputManager::GetAxis(const std::string& axis, int gamepadId) const {
    auto it = axisMapping.find(axis);
    if (it == axisMapping.end()) return 0.0f;

    return GetGamepadAxis(gamepadId, it->second);
}

} // namespace ptx
