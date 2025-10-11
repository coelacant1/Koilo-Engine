#include <ptx/systems/input/mouse.hpp>

namespace ptx {

Mouse::Mouse() {
    currentButtons.fill(false);
    previousButtons.fill(false);
    position = Vector2D(0.0f, 0.0f);
    previousPosition = Vector2D(0.0f, 0.0f);
    delta = Vector2D(0.0f, 0.0f);
    scrollDelta = Vector2D(0.0f, 0.0f);
}

void Mouse::Update() {
    // Calculate delta for this frame
    delta = position - previousPosition;
    previousPosition = position;

    // Copy current button state to previous
    previousButtons = currentButtons;

    // Reset scroll delta (scroll is per-frame, not persistent)
    scrollDelta = Vector2D(0.0f, 0.0f);
}

void Mouse::SetPosition(float x, float y) {
    position.X = x;
    position.Y = y;
}

void Mouse::SetButtonState(MouseButton button, bool pressed) {
    size_t index = static_cast<size_t>(button);
    if (index < currentButtons.size()) {
        currentButtons[index] = pressed;
    }
}

void Mouse::AddScrollDelta(float x, float y) {
    scrollDelta.X += x;
    scrollDelta.Y += y;
}

bool Mouse::IsButtonPressed(MouseButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= currentButtons.size()) return false;

    return currentButtons[index] && !previousButtons[index];
}

bool Mouse::IsButtonHeld(MouseButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= currentButtons.size()) return false;

    return currentButtons[index];
}

bool Mouse::IsButtonReleased(MouseButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= currentButtons.size()) return false;

    return !currentButtons[index] && previousButtons[index];
}

} // namespace ptx
