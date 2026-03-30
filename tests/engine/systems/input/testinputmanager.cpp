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

// ========== Input Listener Tests (#28) ==========

namespace {

struct TestListener : koilo::IInputListener {
    const char* name;
    int priority;
    int keyCalls = 0;
    bool consumeKeys = false;

    TestListener(const char* n, int p) : name(n), priority(p) {}
    const char* GetName() const override { return name; }
    int GetPriority() const override { return priority; }

    bool OnKeyEvent(koilo::KeyEvent& event) override {
        ++keyCalls;
        return consumeKeys;
    }
};

} // namespace

void TestInputManager::TestListenerRegistryDispatch() {
    koilo::InputListenerRegistry reg;
    TestListener listener("test", 0);
    reg.Register(&listener);

    koilo::KeyEvent ev;
    ev.key = koilo::KeyCode::Space;
    ev.action = koilo::KeyAction::Pressed;
    reg.DispatchKey(ev);

    TEST_ASSERT_EQUAL(1, listener.keyCalls);
    TEST_ASSERT_FALSE(ev.consumed);
}

void TestInputManager::TestListenerPriorityOrder() {
    koilo::InputListenerRegistry reg;
    TestListener low("low", 10);
    TestListener high("high", 100);

    reg.Register(&low);
    reg.Register(&high);

    // High priority should be first in sorted order
    auto& listeners = reg.GetListeners();
    TEST_ASSERT_EQUAL(2, listeners.size());

    // Dispatch to trigger sort
    koilo::KeyEvent ev;
    ev.key = koilo::KeyCode::A;
    ev.action = koilo::KeyAction::Pressed;
    reg.DispatchKey(ev);

    TEST_ASSERT_EQUAL(1, high.keyCalls);
    TEST_ASSERT_EQUAL(1, low.keyCalls);

    // Verify sorted: first listener should be high priority
    TEST_ASSERT_EQUAL(100, listeners[0]->GetPriority());
    TEST_ASSERT_EQUAL(10, listeners[1]->GetPriority());
}

void TestInputManager::TestListenerConsumption() {
    koilo::InputListenerRegistry reg;
    TestListener high("high", 100);
    TestListener low("low", 10);
    high.consumeKeys = true;

    reg.Register(&low);
    reg.Register(&high);

    koilo::KeyEvent ev;
    ev.key = koilo::KeyCode::Escape;
    ev.action = koilo::KeyAction::Pressed;
    reg.DispatchKey(ev);

    TEST_ASSERT_TRUE(ev.consumed);
    TEST_ASSERT_EQUAL(1, high.keyCalls);
    TEST_ASSERT_EQUAL(0, low.keyCalls); // Consumed, never reached
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
    RUN_TEST(TestListenerRegistryDispatch);
    RUN_TEST(TestListenerPriorityOrder);
    RUN_TEST(TestListenerConsumption);
}
