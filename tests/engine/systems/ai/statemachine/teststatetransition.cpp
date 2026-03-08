// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file teststatetransition.cpp
 * @brief Implementation of StateTransition unit tests.
 */

#include "teststatetransition.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestStateTransition::TestDefaultConstructor() {
    StateTransition transition("Target", []() { return false; });
    TEST_ASSERT_TRUE(true);
}

void TestStateTransition::TestParameterizedConstructor() {
    bool shouldTransition = false;
    StateTransition transition("StateB", [&]() { return shouldTransition; });
    TEST_ASSERT_FALSE(transition.condition());
    shouldTransition = true;
    TEST_ASSERT_TRUE(transition.condition());
}

// ========== Edge Cases ==========

void TestStateTransition::TestEdgeCases() {
    // Always-true condition
    StateTransition alwaysTrue("Target", []() { return true; });
    TEST_ASSERT_TRUE(alwaysTrue.condition());

    // Always-false condition
    StateTransition alwaysFalse("Target", []() { return false; });
    TEST_ASSERT_FALSE(alwaysFalse.condition());
}
// ========== Test Runner ==========

void TestStateTransition::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
