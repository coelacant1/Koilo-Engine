/**
 * @file inputmanager.hpp
 * @brief Central input management system for keyboard, mouse, and gamepad input.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include "keyboard.hpp"
#include "mouse.hpp"
#include "gamepad.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @class InputManager
 * @brief Central manager for all input devices with action mapping support.
 */
class InputManager {
private:
    Keyboard keyboard;
    Mouse mouse;
    std::unordered_map<int, Gamepad> gamepads;

    // Action mapping: action name -> list of key codes
    std::unordered_map<std::string, KeyCode> actionToKey;
    std::unordered_map<std::string, MouseButton> actionToMouseButton;
    std::unordered_map<std::string, GamepadButton> actionToGamepadButton;

    // Axis mapping: axis name -> gamepad axis
    std::unordered_map<std::string, GamepadAxis> axisMapping;

    // Maximum gamepads to support
    static constexpr int MaxGamepads = 4;

public:
    /**
     * @brief Default constructor.
     */
    InputManager();

    /**
     * @brief Updates all input devices. Call once per frame.
     */
    void Update();

    // === Keyboard Access ===

    /**
     * @brief Checks if a key was pressed this frame.
     */
    bool IsKeyPressed(KeyCode key) const { return keyboard.IsKeyPressed(key); }

    /**
     * @brief Checks if a key is currently held.
     */
    bool IsKeyHeld(KeyCode key) const { return keyboard.IsKeyHeld(key); }

    /**
     * @brief Checks if a key was released this frame.
     */
    bool IsKeyReleased(KeyCode key) const { return keyboard.IsKeyReleased(key); }

    /**
     * @brief Gets keyboard text input.
     */
    std::string GetTextInput() { return keyboard.GetTextInput(); }

    /**
     * @brief Gets reference to keyboard for platform integration.
     */
    Keyboard& GetKeyboard() { return keyboard; }

    // === Mouse Access ===

    /**
     * @brief Gets the mouse position.
     */
    Vector2D GetMousePosition() const { return mouse.GetPosition(); }

    /**
     * @brief Gets the mouse movement delta.
     */
    Vector2D GetMouseDelta() const { return mouse.GetDelta(); }

    /**
     * @brief Gets the scroll wheel delta.
     */
    Vector2D GetScrollDelta() const { return mouse.GetScrollDelta(); }

    /**
     * @brief Checks if a mouse button was pressed this frame.
     */
    bool IsMouseButtonPressed(MouseButton button) const { return mouse.IsButtonPressed(button); }

    /**
     * @brief Checks if a mouse button is held.
     */
    bool IsMouseButtonHeld(MouseButton button) const { return mouse.IsButtonHeld(button); }

    /**
     * @brief Checks if a mouse button was released this frame.
     */
    bool IsMouseButtonReleased(MouseButton button) const { return mouse.IsButtonReleased(button); }

    /**
     * @brief Gets reference to mouse for platform integration.
     */
    Mouse& GetMouse() { return mouse; }

    // === Gamepad Access ===

    /**
     * @brief Gets a gamepad by ID, creating it if it doesn't exist.
     * @param id The gamepad ID (0-3).
     * @return Reference to the gamepad.
     */
    Gamepad& GetGamepad(int id);

    /**
     * @brief Checks if a gamepad is connected.
     * @param id The gamepad ID.
     */
    bool IsGamepadConnected(int id) const;

    /**
     * @brief Checks if a gamepad button was pressed this frame.
     * @param id The gamepad ID.
     * @param button The button to check.
     */
    bool IsGamepadButtonPressed(int id, GamepadButton button) const;

    /**
     * @brief Checks if a gamepad button is held.
     * @param id The gamepad ID.
     * @param button The button to check.
     */
    bool IsGamepadButtonHeld(int id, GamepadButton button) const;

    /**
     * @brief Gets a gamepad axis value.
     * @param id The gamepad ID.
     * @param axis The axis to get.
     */
    float GetGamepadAxis(int id, GamepadAxis axis) const;

    // === Action Mapping ===

    /**
     * @brief Maps an action name to a key.
     * @param action The action name (e.g., "Jump", "Fire", "Pause").
     * @param key The key to bind.
     */
    void MapAction(const std::string& action, KeyCode key);

    /**
     * @brief Maps an action name to a mouse button.
     * @param action The action name.
     * @param button The mouse button to bind.
     */
    void MapAction(const std::string& action, MouseButton button);

    /**
     * @brief Maps an action name to a gamepad button.
     * @param action The action name.
     * @param button The gamepad button to bind.
     */
    void MapAction(const std::string& action, GamepadButton button);

    /**
     * @brief Checks if an action was pressed this frame.
     * @param action The action name.
     * @return True if any mapped input for this action was pressed.
     */
    bool IsActionPressed(const std::string& action) const;

    /**
     * @brief Checks if an action is held.
     * @param action The action name.
     * @return True if any mapped input for this action is held.
     */
    bool IsActionHeld(const std::string& action) const;

    // === Axis Mapping ===

    /**
     * @brief Maps an axis name to a gamepad axis.
     * @param axis The axis name (e.g., "MoveHorizontal", "MoveVertical").
     * @param gamepadAxis The gamepad axis to bind.
     */
    void MapAxis(const std::string& axis, GamepadAxis gamepadAxis);

    /**
     * @brief Gets an axis value.
     * @param axis The axis name.
     * @param gamepadId The gamepad ID to read from (default 0).
     * @return The axis value (-1.0 to 1.0).
     */
    float GetAxis(const std::string& axis, int gamepadId = 0) const;

    PTX_BEGIN_FIELDS(InputManager)
        /* No reflected fields */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(InputManager)
        PTX_METHOD_AUTO(InputManager, Update, "Update"),
        PTX_METHOD_AUTO(InputManager, IsKeyPressed, "Is key pressed"),
        PTX_METHOD_AUTO(InputManager, IsKeyHeld, "Is key held"),
        PTX_METHOD_AUTO(InputManager, GetMousePosition, "Get mouse position"),
        PTX_METHOD_AUTO(InputManager, GetMouseDelta, "Get mouse delta"),
        PTX_METHOD_AUTO(InputManager, IsMouseButtonPressed, "Is mouse button pressed"),
        PTX_METHOD_AUTO(InputManager, IsGamepadConnected, "Is gamepad connected"),
        PTX_METHOD_AUTO(InputManager, IsActionPressed, "Is action pressed"),
        PTX_METHOD_AUTO(InputManager, IsActionHeld, "Is action held"),
        PTX_METHOD_AUTO(InputManager, GetAxis, "Get axis")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(InputManager)
        PTX_CTOR0(InputManager)
    PTX_END_DESCRIBE(InputManager)
};

} // namespace ptx
