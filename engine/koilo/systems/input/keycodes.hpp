/**
 * @file keycodes.hpp
 * @brief Standard keyboard key codes for the input system.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @enum KeyCode
 * @brief Enumeration of keyboard keys following SDL/GLFW conventions.
 */
enum class KeyCode : uint16_t {
    Unknown = 0,

    // Alphanumeric keys
    A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10, H = 11,
    I = 12, J = 13, K = 14, L = 15, M = 16, N = 17, O = 18, P = 19,
    Q = 20, R = 21, S = 22, T = 23, U = 24, V = 25, W = 26, X = 27,
    Y = 28, Z = 29,

    Num1 = 30, Num2 = 31, Num3 = 32, Num4 = 33, Num5 = 34,
    Num6 = 35, Num7 = 36, Num8 = 37, Num9 = 38, Num0 = 39,

    // Function keys
    F1 = 58, F2 = 59, F3 = 60, F4 = 61, F5 = 62, F6 = 63,
    F7 = 64, F8 = 65, F9 = 66, F10 = 67, F11 = 68, F12 = 69,

    // Special keys
    Return = 40,
    Escape = 41,
    Backspace = 42,
    Tab = 43,
    Space = 44,
    Minus = 45,
    Equals = 46,
    LeftBracket = 47,
    RightBracket = 48,
    Backslash = 49,
    Semicolon = 51,
    Apostrophe = 52,
    Grave = 53,
    Comma = 54,
    Period = 55,
    Slash = 56,
    CapsLock = 57,

    // Arrow keys
    Right = 79,
    Left = 80,
    Down = 81,
    Up = 82,

    // Numpad
    NumpadDivide = 84,
    NumpadMultiply = 85,
    NumpadMinus = 86,
    NumpadPlus = 87,
    NumpadEnter = 88,
    Numpad1 = 89, Numpad2 = 90, Numpad3 = 91,
    Numpad4 = 92, Numpad5 = 93, Numpad6 = 94,
    Numpad7 = 95, Numpad8 = 96, Numpad9 = 97,
    Numpad0 = 98,
    NumpadPeriod = 99,

    // Modifier keys
    LeftControl = 224,
    LeftShift = 225,
    LeftAlt = 226,
    LeftMeta = 227,
    RightControl = 228,
    RightShift = 229,
    RightAlt = 230,
    RightMeta = 231,

    // System keys
    PrintScreen = 70,
    ScrollLock = 71,
    Pause = 72,
    Insert = 73,
    Home = 74,
    PageUp = 75,
    Delete = 76,
    End = 77,
    PageDown = 78,

    // Max key code
    MaxKeyCode = 512

    PTX_BEGIN_FIELDS(KeyCode)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(KeyCode)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(KeyCode)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(KeyCode)

};

/**
 * @enum MouseButton
 * @brief Enumeration of mouse buttons.
 */
enum class MouseButton : uint8_t {
    Left = 0,
    Middle = 1,
    Right = 2,
    X1 = 3,
    X2 = 4,
    MaxButton = 8

    PTX_BEGIN_FIELDS(MouseButton)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MouseButton)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MouseButton)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(MouseButton)

};

/**
 * @enum GamepadButton
 * @brief Enumeration of standard gamepad buttons (Xbox-style layout).
 */
enum class GamepadButton : uint8_t {
    A = 0,              // Xbox A / PS Cross
    B = 1,              // Xbox B / PS Circle
    X = 2,              // Xbox X / PS Square
    Y = 3,              // Xbox Y / PS Triangle
    LeftBumper = 4,
    RightBumper = 5,
    Back = 6,           // Xbox Back / PS Share
    Start = 7,          // Xbox Start / PS Options
    Guide = 8,          // Xbox Guide / PS Home
    LeftStick = 9,      // Left stick click
    RightStick = 10,    // Right stick click
    DPadUp = 11,
    DPadDown = 12,
    DPadLeft = 13,
    DPadRight = 14,
    MaxButton = 16

    PTX_BEGIN_FIELDS(GamepadButton)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GamepadButton)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GamepadButton)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(GamepadButton)

};

/**
 * @enum GamepadAxis
 * @brief Enumeration of gamepad analog axes.
 */
enum class GamepadAxis : uint8_t {
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    LeftTrigger = 4,
    RightTrigger = 5,
    MaxAxis = 6

    PTX_BEGIN_FIELDS(GamepadAxis)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GamepadAxis)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GamepadAxis)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(GamepadAxis)

};

} // namespace ptx
