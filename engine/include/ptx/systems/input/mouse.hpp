/**
 * @file mouse.hpp
 * @brief Mouse input handling including position, buttons, and scroll.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <array>
#include "keycodes.hpp"
#include "../../../core/math/vector2d.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @class Mouse
 * @brief Manages mouse input state including position, buttons, and scroll.
 */
class Mouse {
private:
    // Mouse position
    Vector2D position;
    Vector2D previousPosition;
    Vector2D delta;

    // Button states
    std::array<bool, static_cast<size_t>(MouseButton::MaxButton)> currentButtons;
    std::array<bool, static_cast<size_t>(MouseButton::MaxButton)> previousButtons;

    // Scroll wheel
    Vector2D scrollDelta;

    // Cursor state
    bool cursorVisible = true;
    bool cursorLocked = false;

public:
    /**
     * @brief Default constructor.
     */
    Mouse();

    /**
     * @brief Updates mouse state. Call once per frame before processing input.
     */
    void Update();

    /**
     * @brief Sets the mouse position (called by platform layer).
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     */
    void SetPosition(float x, float y);

    /**
     * @brief Sets the state of a mouse button (called by platform layer).
     * @param button The mouse button.
     * @param pressed True if pressed, false if released.
     */
    void SetButtonState(MouseButton button, bool pressed);

    /**
     * @brief Adds scroll wheel delta (called by platform layer).
     * @param x Horizontal scroll delta.
     * @param y Vertical scroll delta.
     */
    void AddScrollDelta(float x, float y);

    /**
     * @brief Gets the current mouse position.
     * @return The mouse position in screen coordinates.
     */
    Vector2D GetPosition() const { return position; }

    /**
     * @brief Gets the mouse position delta since last frame.
     * @return The mouse movement delta.
     */
    Vector2D GetDelta() const { return delta; }

    /**
     * @brief Gets the scroll wheel delta for this frame.
     * @return The scroll delta (x = horizontal, y = vertical).
     */
    Vector2D GetScrollDelta() const { return scrollDelta; }

    /**
     * @brief Checks if a mouse button was pressed this frame.
     * @param button The button to check.
     * @return True if the button was just pressed.
     */
    bool IsButtonPressed(MouseButton button) const;

    /**
     * @brief Checks if a mouse button is currently held down.
     * @param button The button to check.
     * @return True if the button is held.
     */
    bool IsButtonHeld(MouseButton button) const;

    /**
     * @brief Checks if a mouse button was released this frame.
     * @param button The button to check.
     * @return True if the button was just released.
     */
    bool IsButtonReleased(MouseButton button) const;

    /**
     * @brief Sets whether the cursor is visible.
     * @param visible True to show cursor, false to hide.
     */
    void SetCursorVisible(bool visible) { cursorVisible = visible; }

    /**
     * @brief Checks if the cursor is visible.
     */
    bool IsCursorVisible() const { return cursorVisible; }

    /**
     * @brief Sets whether the cursor is locked (for FPS-style control).
     * @param locked True to lock cursor to window center.
     */
    void SetCursorLocked(bool locked) { cursorLocked = locked; }

    /**
     * @brief Checks if the cursor is locked.
     */
    bool IsCursorLocked() const { return cursorLocked; }

    PTX_BEGIN_FIELDS(Mouse)
        PTX_FIELD(Mouse, position, "Position", 0, 0),
        PTX_FIELD(Mouse, cursorVisible, "Cursor visible", 0, 1),
        PTX_FIELD(Mouse, cursorLocked, "Cursor locked", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Mouse)
        PTX_METHOD_AUTO(Mouse, Update, "Update"),
        PTX_METHOD_AUTO(Mouse, GetPosition, "Get position"),
        PTX_METHOD_AUTO(Mouse, GetDelta, "Get delta"),
        PTX_METHOD_AUTO(Mouse, GetScrollDelta, "Get scroll delta"),
        PTX_METHOD_AUTO(Mouse, IsButtonPressed, "Is button pressed"),
        PTX_METHOD_AUTO(Mouse, IsButtonHeld, "Is button held"),
        PTX_METHOD_AUTO(Mouse, IsButtonReleased, "Is button released")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Mouse)
        PTX_CTOR0(Mouse)
    PTX_END_DESCRIBE(Mouse)
};

} // namespace ptx
