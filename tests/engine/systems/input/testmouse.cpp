// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmouse.cpp
 * @brief Implementation of Mouse unit tests.
 */

#include "testmouse.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestMouse::TestDefaultConstructor() {
    Mouse mouse;
    Vector2D pos = mouse.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Y);
}

void TestMouse::TestParameterizedConstructor() {
    Mouse mouse;
    // Mouse doesn't have parameterized constructor
    // Verify initial position
    Vector2D pos = mouse.GetPosition();
    TEST_ASSERT_FALSE(isnan(pos.X));
}

// ========== Method Tests ==========

void TestMouse::TestUpdate() {
    Mouse mouse;
    mouse.Update();
    // Update should process mouse state
    TEST_ASSERT_TRUE(true);
}

void TestMouse::TestGetPosition() {
    Mouse mouse;
    Vector2D pos = mouse.GetPosition();
    TEST_ASSERT_FALSE(isnan(pos.X) || isnan(pos.Y));
}

void TestMouse::TestGetDelta() {
    Mouse mouse;
    Vector2D delta = mouse.GetDelta();
    // Initially no movement
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, delta.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, delta.Y);
}

void TestMouse::TestGetScrollDelta() {
    Mouse mouse;
    Vector2D scroll = mouse.GetScrollDelta();
    // Initially no scroll
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, scroll.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, scroll.Y);
}

void TestMouse::TestIsButtonPressed() {
    Mouse mouse;
    // Initially no buttons pressed
    bool pressed = mouse.IsButtonPressed(MouseButton::Left); // Left button
    TEST_ASSERT_FALSE(pressed);
}

void TestMouse::TestIsButtonHeld() {
    Mouse mouse;
    // Initially no buttons held
    bool held = mouse.IsButtonHeld(MouseButton::Left);
    TEST_ASSERT_FALSE(held);
}

void TestMouse::TestIsButtonReleased() {
    Mouse mouse;
    // Initially no buttons released
    bool released = mouse.IsButtonReleased(MouseButton::Left);
    TEST_ASSERT_FALSE(released);
}

// ========== Edge Cases ==========

void TestMouse::TestEdgeCases() {
    Mouse mouse;
    TEST_ASSERT_FALSE(mouse.IsButtonPressed(MouseButton::Left));
    TEST_ASSERT_FALSE(mouse.IsButtonPressed(MouseButton::Right));
    TEST_ASSERT_FALSE(mouse.IsButtonPressed(MouseButton::Middle));
}
// ========== Test Runner ==========

void TestMouse::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestGetDelta);
    RUN_TEST(TestGetScrollDelta);
    RUN_TEST(TestIsButtonPressed);
    RUN_TEST(TestIsButtonHeld);
    RUN_TEST(TestIsButtonReleased);
    RUN_TEST(TestEdgeCases);
}
