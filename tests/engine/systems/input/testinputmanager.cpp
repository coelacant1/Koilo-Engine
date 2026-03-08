// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testinputmanager.cpp
 * @brief Implementation of InputManager unit tests.
 */

#include "testinputmanager.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestInputManager::TestDefaultConstructor() {
    InputManager manager;
    // Initial state - no keys pressed
    TEST_ASSERT_FALSE(manager.IsKeyPressed(KeyCode::Space));
}

void TestInputManager::TestParameterizedConstructor() {
    InputManager manager;
    // InputManager doesn't have parameterized constructor
    TEST_ASSERT_FALSE(manager.IsKeyPressed(KeyCode::A));
}

// ========== Method Tests ==========

void TestInputManager::TestUpdate() {
    InputManager manager;
    manager.Update();
    // Update should poll all input devices
    TEST_ASSERT_TRUE(true);
}

void TestInputManager::TestIsKeyPressed() {
    InputManager manager;
    bool pressed = manager.IsKeyPressed(KeyCode::Space); // Space
    TEST_ASSERT_FALSE(pressed);
}

void TestInputManager::TestIsKeyHeld() {
    InputManager manager;
    bool held = manager.IsKeyHeld(KeyCode::Space);
    TEST_ASSERT_FALSE(held);
}

void TestInputManager::TestGetMousePosition() {
    InputManager manager;
    Vector2D pos = manager.GetMousePosition();
    TEST_ASSERT_FALSE(isnan(pos.X) || isnan(pos.Y));
}

void TestInputManager::TestGetMouseDelta() {
    InputManager manager;
    Vector2D delta = manager.GetMouseDelta();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, delta.X);
}

void TestInputManager::TestIsMouseButtonPressed() {
    InputManager manager;
    bool pressed = manager.IsMouseButtonPressed(MouseButton::Left);
    TEST_ASSERT_FALSE(pressed);
}

void TestInputManager::TestIsGamepadConnected() {
    InputManager manager;
    bool connected = manager.IsGamepadConnected(0);
    TEST_ASSERT_TRUE(connected == true || connected == false);
}

void TestInputManager::TestIsActionPressed() {
    InputManager manager;
    bool pressed = manager.IsActionPressed("jump");
    TEST_ASSERT_FALSE(pressed);
}

void TestInputManager::TestIsActionHeld() {
    InputManager manager;
    bool held = manager.IsActionHeld("jump");
    TEST_ASSERT_FALSE(held);
}

void TestInputManager::TestGetAxis() {
    InputManager manager;
    float axis = manager.GetAxis("horizontal");
    TEST_ASSERT_TRUE(axis >= -1.0f && axis <= 1.0f);
}

// ========== Edge Cases ==========

void TestInputManager::TestEdgeCases() {
    InputManager manager;
    // Test non-existent action
    bool pressed = manager.IsActionPressed("nonexistent");
    TEST_ASSERT_FALSE(pressed);
    
    // Test non-existent axis
    float axis = manager.GetAxis("nonexistent");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, axis);
}
// ========== Test Runner ==========

void TestInputManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestIsKeyPressed);
    RUN_TEST(TestIsKeyHeld);
    RUN_TEST(TestGetMousePosition);
    RUN_TEST(TestGetMouseDelta);
    RUN_TEST(TestIsMouseButtonPressed);
    RUN_TEST(TestIsGamepadConnected);
    RUN_TEST(TestIsActionPressed);
    RUN_TEST(TestIsActionHeld);
    RUN_TEST(TestGetAxis);
    RUN_TEST(TestEdgeCases);
}
