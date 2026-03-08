#include <ptx/systems/input/keyboard.hpp>
#include <algorithm>

namespace ptx {

Keyboard::Keyboard() {
    currentKeys.fill(false);
    previousKeys.fill(false);
}

void Keyboard::Update() {
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

void Keyboard::SetKeyState(KeyCode key, bool pressed) {
    size_t index = static_cast<size_t>(key);
    if (index < currentKeys.size()) {
        currentKeys[index] = pressed;
    }
}

bool Keyboard::IsKeyPressed(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= currentKeys.size()) return false;

    // Pressed this frame but not last frame
    return currentKeys[index] && !previousKeys[index];
}

bool Keyboard::IsKeyHeld(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= currentKeys.size()) return false;

    return currentKeys[index];
}

bool Keyboard::IsKeyReleased(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= currentKeys.size()) return false;

    // Released this frame but was pressed last frame
    return !currentKeys[index] && previousKeys[index];
}

void Keyboard::AddTextInput(const std::string& character) {
    textInput += character;
}

std::string Keyboard::GetTextInput() {
    std::string result = textInput;
    textInput.clear();
    return result;
}

void Keyboard::ClearTextInput() {
    textInput.clear();
}

} // namespace ptx
