// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file keyboard.hpp
 * @brief Keyboard input handling and key state tracking.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <array>
#include <string>
#include "keycodes.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class Keyboard
 * @brief Manages keyboard input state and provides key queries.
 */
class Keyboard {
private:
    // Current frame key states
    std::array<bool, static_cast<size_t>(KeyCode::MaxKeyCode)> currentKeys;

    // Previous frame key states (for detecting press/release)
    std::array<bool, static_cast<size_t>(KeyCode::MaxKeyCode)> previousKeys;

    // Text input buffer for typing
    std::string textInput;

    // Modifier key states
    bool shiftPressed = false;
    bool ctrlPressed = false;
    bool altPressed = false;
    bool metaPressed = false;

public:
    /**
     * @brief Default constructor.
     */
    Keyboard();

    /**
     * @brief Updates keyboard state. Call once per frame before processing input.
     */
    void Update();

    /**
     * @brief Sets the state of a key (called by platform layer).
     * @param key The key code.
     * @param pressed True if pressed, false if released.
     */
    void SetKeyState(KeyCode key, bool pressed);

    /**
     * @brief Checks if a key was pressed this frame (down event).
     * @param key The key to check.
     * @return True if the key was just pressed.
     */
    bool IsKeyPressed(KeyCode key) const;

    /**
     * @brief Checks if a key is currently held down.
     * @param key The key to check.
     * @return True if the key is held.
     */
    bool IsKeyHeld(KeyCode key) const;

    /**
     * @brief Checks if a key was released this frame (up event).
     * @param key The key to check.
     * @return True if the key was just released.
     */
    bool IsKeyReleased(KeyCode key) const;

    /**
     * @brief Adds a character to the text input buffer.
     * @param character The UTF-8 character to add.
     */
    void AddTextInput(const std::string& character);

    /**
     * @brief Gets and clears the text input buffer.
     * @return The accumulated text input since last call.
     */
    std::string GetTextInput();

    /**
     * @brief Clears the text input buffer.
     */
    void ClearTextInput();

    /**
     * @brief Checks if Shift is pressed.
     */
    bool IsShiftPressed() const { return shiftPressed; }

    /**
     * @brief Checks if Ctrl is pressed.
     */
    bool IsCtrlPressed() const { return ctrlPressed; }

    /**
     * @brief Checks if Alt is pressed.
     */
    bool IsAltPressed() const { return altPressed; }

    /**
     * @brief Checks if Meta/Windows/Command key is pressed.
     */
    bool IsMetaPressed() const { return metaPressed; }

    KL_BEGIN_FIELDS(Keyboard)
        /* No reflected fields */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Keyboard)
        KL_METHOD_AUTO(Keyboard, Update, "Update"),
        KL_METHOD_AUTO(Keyboard, IsKeyPressed, "Is key pressed"),
        KL_METHOD_AUTO(Keyboard, IsKeyHeld, "Is key held"),
        KL_METHOD_AUTO(Keyboard, IsKeyReleased, "Is key released"),
        KL_METHOD_AUTO(Keyboard, GetTextInput, "Get text input"),
        KL_METHOD_AUTO(Keyboard, ClearTextInput, "Clear text input")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Keyboard)
        KL_CTOR0(Keyboard)
    KL_END_DESCRIBE(Keyboard)
};

} // namespace koilo
