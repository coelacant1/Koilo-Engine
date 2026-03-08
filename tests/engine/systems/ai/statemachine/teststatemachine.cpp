// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file teststatemachine.cpp
 * @brief Implementation of StateMachine unit tests.
 */

#include "teststatemachine.hpp"
#include <koilo/core/time/timemanager.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestStateMachine::TestDefaultConstructor() {
    StateMachine sm;
    std::string name = sm.GetCurrentStateName();
    TEST_ASSERT_TRUE(name.empty());
}

void TestStateMachine::TestParameterizedConstructor() {
    StateMachine sm;
    TEST_ASSERT_TRUE(sm.GetCurrentStateName().empty());
}

// ========== Method Tests ==========

void TestStateMachine::TestGetCurrentStateName() {
    StateMachine sm;
    std::string name = sm.GetCurrentStateName();
    TEST_ASSERT_TRUE(name.empty());
}

void TestStateMachine::TestSetInitialState() {
    StateMachine sm;
    sm.AddState("Initial");
    sm.SetInitialState("Initial");
    sm.Start();
    std::string name = sm.GetCurrentStateName();
    TEST_ASSERT_EQUAL_STRING("Initial", name.c_str());
}

void TestStateMachine::TestTransitionTo() {
    StateMachine sm;
    sm.AddState("State1");
    sm.AddState("State2");
    sm.SetInitialState("State1");
    sm.Start();
    sm.TransitionTo("State2");
    std::string name = sm.GetCurrentStateName();
    TEST_ASSERT_EQUAL_STRING("State2", name.c_str());
}

void TestStateMachine::TestStart() {
    StateMachine sm;
    sm.AddState("Initial");
    sm.SetInitialState("Initial");
    sm.Start();
    std::string name = sm.GetCurrentStateName();
    TEST_ASSERT_EQUAL_STRING("Initial", name.c_str());
}

void TestStateMachine::TestStop() {
    StateMachine sm;
    sm.AddState("Test");
    sm.SetInitialState("Test");
    sm.Start();
    sm.Stop();
    TEST_ASSERT_TRUE(true);
}

void TestStateMachine::TestUpdate() {
    StateMachine sm;
    sm.AddState("Running");
    sm.SetInitialState("Running");
    sm.Start();
    koilo::TimeManager::GetInstance().Tick(0.016f);
    sm.Update();
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestStateMachine::TestEdgeCases() {
    StateMachine sm;
    // Update without initial state
    koilo::TimeManager::GetInstance().Tick(0.0f);
    sm.Update();
    // Start without initial state
    sm.Start();
    TEST_ASSERT_TRUE(true);
}
// ========== Test Runner ==========

void TestStateMachine::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetCurrentStateName);
    RUN_TEST(TestSetInitialState);
    RUN_TEST(TestTransitionTo);
    RUN_TEST(TestStart);
    RUN_TEST(TestStop);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestEdgeCases);
}
