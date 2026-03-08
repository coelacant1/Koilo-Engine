// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file gamepad.hpp
 * @brief Gamepad/controller input with button and axis mapping.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <array>
#include "keycodes.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class Gamepad
 * @brief Manages a single gamepad/controller input state.
 */
class Gamepad {
private:
    // Gamepad ID
    int id = -1;

    // Connection state
    bool connected = false;

    // Button states
    std::array<bool, static_cast<size_t>(GamepadButton::MaxButton)> currentButtons;
    std::array<bool, static_cast<size_t>(GamepadButton::MaxButton)> previousButtons;

    // Axis values (-1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
    std::array<float, static_cast<size_t>(GamepadAxis::MaxAxis)> axes;

    // Dead zone for analog sticks
    float deadZone = 0.15f;

public:
    /**
     * @brief Default constructor.
     */
    Gamepad();

    /**
     * @brief Constructor with ID.
     * @param id The gamepad ID.
     */
    explicit Gamepad(int id);

    /**
     * @brief Updates gamepad state. Call once per frame.
     */
    void Update();

    /**
     * @brief Sets the connection state.
     * @param connected True if connected, false if disconnected.
     */
    void SetConnected(bool connected) { this->connected = connected; }

    /**
     * @brief Checks if the gamepad is connected.
     */
    bool IsConnected() const { return connected; }

    /**
     * @brief Gets the gamepad ID.
     */
    int GetID() const { return id; }

    /**
     * @brief Sets the state of a button (called by platform layer).
     * @param button The button.
     * @param pressed True if pressed, false if released.
     */
    void SetButtonState(GamepadButton button, bool pressed);

    /**
     * @brief Sets the value of an axis (called by platform layer).
     * @param axis The axis.
     * @param value The axis value (-1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers).
     */
    void SetAxisValue(GamepadAxis axis, float value);

    /**
     * @brief Checks if a button was pressed this frame.
     * @param button The button to check.
     * @return True if the button was just pressed.
     */
    bool IsButtonPressed(GamepadButton button) const;

    /**
     * @brief Checks if a button is currently held down.
     * @param button The button to check.
     * @return True if the button is held.
     */
    bool IsButtonHeld(GamepadButton button) const;

    /**
     * @brief Checks if a button was released this frame.
     * @param button The button to check.
     * @return True if the button was just released.
     */
    bool IsButtonReleased(GamepadButton button) const;

    /**
     * @brief Gets the value of an axis with dead zone applied.
     * @param axis The axis to get.
     * @return The axis value after dead zone filtering.
     */
    float GetAxisValue(GamepadAxis axis) const;

    /**
     * @brief Sets the dead zone for analog sticks.
     * @param deadZone The dead zone threshold (0.0 to 1.0).
     */
    void SetDeadZone(float deadZone) { this->deadZone = deadZone; }

    /**
     * @brief Gets the dead zone value.
     */
    float GetDeadZone() const { return deadZone; }

    KL_BEGIN_FIELDS(Gamepad)
        KL_FIELD(Gamepad, id, "Id", -1, 16),
        KL_FIELD(Gamepad, connected, "Connected", 0, 1),
        KL_FIELD(Gamepad, deadZone, "Dead zone", 0.0f, 1.0f)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Gamepad)
        KL_METHOD_AUTO(Gamepad, Update, "Update"),
        KL_METHOD_AUTO(Gamepad, IsConnected, "Is connected"),
        KL_METHOD_AUTO(Gamepad, GetID, "Get id"),
        KL_METHOD_AUTO(Gamepad, IsButtonPressed, "Is button pressed"),
        KL_METHOD_AUTO(Gamepad, IsButtonHeld, "Is button held"),
        KL_METHOD_AUTO(Gamepad, IsButtonReleased, "Is button released"),
        KL_METHOD_AUTO(Gamepad, GetAxisValue, "Get axis value")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Gamepad)
        KL_CTOR0(Gamepad),
        KL_CTOR(Gamepad, int)
    KL_END_DESCRIBE(Gamepad)
};

} // namespace koilo
