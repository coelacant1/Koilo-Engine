// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/input/keyboard.hpp>

namespace koilo {

koilo::Keyboard::Keyboard() {
    currentKeys.fill(false);
    previousKeys.fill(false);
}

void koilo::Keyboard::Update() {
    // Copy current state to previous for next frame
    previousKeys = currentKeys;

    // Update modifier states
    shiftPressed = currentKeys[static_cast<size_t>(KeyCode::LeftShift)] ||
                   currentKeys[static_cast<size_t>(KeyCode::RightShift)];
    ctrlPressed = currentKeys[static_cast<size_t>(KeyCode::LeftControl)] ||
                  currentKeys[static_cast<size_t>(KeyCode::RightControl)];
    altPressed = currentKeys[static_cast<size_t>(KeyCode::LeftAlt)] ||
                 currentKeys[static_cast<size_t>(KeyCode::RightAlt)];
    metaPressed = currentKeys[static_cast<size_t>(KeyCode::LeftMeta)] ||
                  currentKeys[static_cast<size_t>(KeyCode::RightMeta)];
}

void koilo::Keyboard::SetKeyState(KeyCode key, bool pressed) {
    size_t index = static_cast<size_t>(key);
    if (index < currentKeys.size()) {
        currentKeys[index] = pressed;
    }
}

bool koilo::Keyboard::IsKeyPressed(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= currentKeys.size()) return false;

    // Pressed this frame but not last frame
    return currentKeys[index] && !previousKeys[index];
}

bool koilo::Keyboard::IsKeyHeld(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= currentKeys.size()) return false;

    return currentKeys[index];
}

bool koilo::Keyboard::IsKeyReleased(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= currentKeys.size()) return false;

    // Released this frame but was pressed last frame
    return !currentKeys[index] && previousKeys[index];
}

void koilo::Keyboard::AddTextInput(const std::string& character) {
    textInput += character;
}

std::string koilo::Keyboard::GetTextInput() {
    std::string result = textInput;
    textInput.clear();
    return result;
}

void koilo::Keyboard::ClearTextInput() {
    textInput.clear();
}

} // namespace koilo
