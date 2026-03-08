// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file teststate.cpp
 * @brief Implementation of State unit tests.
 */

#include "teststate.hpp"
#include <koilo/core/time/timemanager.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestState::TestDefaultConstructor() {
    State state("TestState");
    std::string name = state.GetName();
    TEST_ASSERT_EQUAL_STRING("TestState", name.c_str());
}

void TestState::TestParameterizedConstructor() {
    State state("InitialState");
    std::string name = state.GetName();
    TEST_ASSERT_EQUAL_STRING("InitialState", name.c_str());
}

// ========== Method Tests ==========

void TestState::TestGetName() {
    State state("MyState");
    std::string name = state.GetName();
    TEST_ASSERT_EQUAL_STRING("MyState", name.c_str());
}

void TestState::TestEnter() {
    State state("TestState");
    state.Enter();
    // Enter should execute without crash
    TEST_ASSERT_TRUE(true);
}

void TestState::TestUpdate() {
    State state("TestState");
    state.Enter();
    koilo::TimeManager::GetInstance().Tick(0.016f); state.Update();
    koilo::TimeManager::GetInstance().Tick(1.0f); state.Update();
    // Update should handle various delta times
    TEST_ASSERT_TRUE(true);
}

void TestState::TestExit() {
    State state("TestState");
    state.Enter();
    state.Exit();
    // Exit should execute without crash
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestState::TestEdgeCases() {
    // Empty name edge case
    State state("");
    std::string name = state.GetName();
    TEST_ASSERT_EQUAL_STRING("", name.c_str());
    
    // Lifecycle without Enter
    State state2("Test");
    koilo::TimeManager::GetInstance().Tick(0.0f); state2.Update();
    state2.Exit();
    TEST_ASSERT_TRUE(true);
}
// ========== Test Runner ==========

void TestState::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetName);
    RUN_TEST(TestEnter);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestExit);
    RUN_TEST(TestEdgeCases);
}
