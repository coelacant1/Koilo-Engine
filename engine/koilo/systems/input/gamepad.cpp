// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/input/gamepad.hpp>
#include <cmath>

namespace koilo {

koilo::Gamepad::Gamepad() : id(-1), connected(false) {
    currentButtons.fill(false);
    previousButtons.fill(false);
    axes.fill(0.0f);
}

koilo::Gamepad::Gamepad(int id) : id(id), connected(false) {
    currentButtons.fill(false);
    previousButtons.fill(false);
    axes.fill(0.0f);
}

void koilo::Gamepad::Update() {
    previousButtons = currentButtons;
}

void koilo::Gamepad::SetButtonState(GamepadButton button, bool pressed) {
    size_t index = static_cast<size_t>(button);
    if (index < currentButtons.size()) {
        currentButtons[index] = pressed;
    }
}

void koilo::Gamepad::SetAxisValue(GamepadAxis axis, float value) {
    size_t index = static_cast<size_t>(axis);
    if (index < axes.size()) {
        axes[index] = value;
    }
}

bool koilo::Gamepad::IsButtonPressed(GamepadButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= currentButtons.size()) return false;

    return currentButtons[index] && !previousButtons[index];
}

bool koilo::Gamepad::IsButtonHeld(GamepadButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= currentButtons.size()) return false;

    return currentButtons[index];
}

bool koilo::Gamepad::IsButtonReleased(GamepadButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= currentButtons.size()) return false;

    return !currentButtons[index] && previousButtons[index];
}

float koilo::Gamepad::GetAxisValue(GamepadAxis axis) const {
    size_t index = static_cast<size_t>(axis);
    if (index >= axes.size()) return 0.0f;

    float value = axes[index];

    // Apply dead zone for stick axes (not triggers)
    if (axis == GamepadAxis::LeftX || axis == GamepadAxis::LeftY ||
        axis == GamepadAxis::RightX || axis == GamepadAxis::RightY) {

        if (std::abs(value) < deadZone) {
            return 0.0f;
        }

        // Re-map value from [deadZone, 1.0] to [0.0, 1.0]
        float sign = value > 0.0f ? 1.0f : -1.0f;
        float absValue = std::abs(value);
        return sign * ((absValue - deadZone) / (1.0f - deadZone));
    }

    return value;
}

} // namespace koilo
