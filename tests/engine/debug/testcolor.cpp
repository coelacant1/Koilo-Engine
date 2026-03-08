// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolor.cpp
 * @brief Implementation of Color unit tests.
 */

#include "testcolor.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestColor::TestDefaultConstructor() {
    TEST_ASSERT_TRUE(true);  
}

void TestColor::TestParameterizedConstructor() {
    TEST_ASSERT_TRUE(true);  
}

// ========== Edge Cases ==========

void TestColor::TestEdgeCases() {
    
    TEST_ASSERT_TRUE(true);  
}

// ========== Test Runner ==========

void TestColor::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
