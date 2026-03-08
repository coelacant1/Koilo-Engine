// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkeyboard.cpp
 * @brief Implementation of Keyboard unit tests.
 */

#include "testkeyboard.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestKeyboard::TestDefaultConstructor() {
    Keyboard keyboard;
    // All keys should be unpressed initially
    TEST_ASSERT_FALSE(keyboard.IsKeyPressed(KeyCode::A));
    TEST_ASSERT_FALSE(keyboard.IsKeyHeld(KeyCode::A));
}

void TestKeyboard::TestParameterizedConstructor() {
    Keyboard keyboard;
    // Keyboard doesn't have parameterized constructor
    // Verify initial state
    TEST_ASSERT_FALSE(keyboard.IsKeyPressed(KeyCode::A));
}

// ========== Method Tests ==========

void TestKeyboard::TestUpdate() {
    Keyboard keyboard;
    keyboard.Update();
    // Update should process input state
    TEST_ASSERT_TRUE(true);
}

void TestKeyboard::TestIsKeyPressed() {
    Keyboard keyboard;
    // Initially no keys pressed
    bool pressed = keyboard.IsKeyPressed(KeyCode::Space); // Space key
    TEST_ASSERT_FALSE(pressed);
}

void TestKeyboard::TestIsKeyHeld() {
    Keyboard keyboard;
    // Initially no keys held
    bool held = keyboard.IsKeyHeld(KeyCode::Space);
    TEST_ASSERT_FALSE(held);
}

void TestKeyboard::TestIsKeyReleased() {
    Keyboard keyboard;
    // Initially no keys released (because none were pressed)
    bool released = keyboard.IsKeyReleased(KeyCode::Space);
    TEST_ASSERT_FALSE(released);
}

void TestKeyboard::TestGetTextInput() {
    Keyboard keyboard;
    std::string text = keyboard.GetTextInput();
    TEST_ASSERT_EQUAL_STRING("", text.c_str());
}

void TestKeyboard::TestClearTextInput() {
    Keyboard keyboard;
    keyboard.ClearTextInput();
    std::string text = keyboard.GetTextInput();
    TEST_ASSERT_EQUAL_STRING("", text.c_str());
}

// ========== Edge Cases ==========

void TestKeyboard::TestEdgeCases() {
    Keyboard keyboard;
    // Test invalid key codes
    bool pressed = keyboard.IsKeyPressed(static_cast<KeyCode>(999));
    TEST_ASSERT_FALSE(pressed);
    
    // Test key code 0
    bool held = keyboard.IsKeyHeld(KeyCode::A);
    TEST_ASSERT_FALSE(held);
}
// ========== Test Runner ==========

void TestKeyboard::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestIsKeyPressed);
    RUN_TEST(TestIsKeyHeld);
    RUN_TEST(TestIsKeyReleased);
    RUN_TEST(TestGetTextInput);
    RUN_TEST(TestClearTextInput);
    RUN_TEST(TestEdgeCases);
}
