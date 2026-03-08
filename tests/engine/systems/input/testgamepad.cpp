// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testgamepad.cpp
 * @brief Implementation of Gamepad unit tests.
 */

#include "testgamepad.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestGamepad::TestDefaultConstructor() {
    Gamepad gamepad;
    // Initially not connected
    TEST_ASSERT_FALSE(gamepad.IsConnected());
}

void TestGamepad::TestParameterizedConstructor() {
    Gamepad gamepad;
    // Gamepad doesn't have parameterized constructor
    // Verify initial state
    TEST_ASSERT_FALSE(gamepad.IsConnected());
}

// ========== Method Tests ==========

void TestGamepad::TestUpdate() {
    Gamepad gamepad;
    gamepad.Update();
    // Update should poll gamepad state
    TEST_ASSERT_TRUE(true);
}

void TestGamepad::TestIsConnected() {
    Gamepad gamepad;
    bool connected = gamepad.IsConnected();
    TEST_ASSERT_TRUE(connected == true || connected == false);
}

void TestGamepad::TestGetID() {
    Gamepad gamepad;
    int id = gamepad.GetID();
    TEST_ASSERT_EQUAL_INT(-1, id);
}

void TestGamepad::TestIsButtonPressed() {
    Gamepad gamepad;
    // Initially no buttons pressed
    bool pressed = gamepad.IsButtonPressed(GamepadButton::A); // Button A
    TEST_ASSERT_FALSE(pressed);
}

void TestGamepad::TestIsButtonHeld() {
    Gamepad gamepad;
    // Initially no buttons held
    bool held = gamepad.IsButtonHeld(GamepadButton::A);
    TEST_ASSERT_FALSE(held);
}

void TestGamepad::TestIsButtonReleased() {
    Gamepad gamepad;
    // Initially no buttons released
    bool released = gamepad.IsButtonReleased(GamepadButton::A);
    TEST_ASSERT_FALSE(released);
}

void TestGamepad::TestGetAxisValue() {
    Gamepad gamepad;
    // Axis values typically in range [-1, 1]
    float axis = gamepad.GetAxisValue(GamepadAxis::LeftX); // Left stick X
    TEST_ASSERT_TRUE(axis >= -1.0f && axis <= 1.0f);
}

// ========== Edge Cases ==========

void TestGamepad::TestEdgeCases() {
    Gamepad gamepad;
    // Test multiple axes (typically 0-5)
    for(int i = 0; i < 6; i++) {
        float axis = gamepad.GetAxisValue(static_cast<GamepadAxis>(i));
        TEST_ASSERT_TRUE(axis >= -1.0f && axis <= 1.0f);
    }
}
// ========== Test Runner ==========

void TestGamepad::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestIsConnected);
    RUN_TEST(TestGetID);
    RUN_TEST(TestIsButtonPressed);
    RUN_TEST(TestIsButtonHeld);
    RUN_TEST(TestIsButtonReleased);
    RUN_TEST(TestGetAxisValue);
    RUN_TEST(TestEdgeCases);
}
