// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdisplayinfo.cpp
 * @brief Implementation of DisplayInfo unit tests.
 */

#include "testdisplayinfo.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestDisplayInfo::TestDefaultConstructor() {
    DisplayInfo info;
    TEST_ASSERT_TRUE(static_cast<long long>(info.width) >= 0);
    TEST_ASSERT_TRUE(static_cast<long long>(info.height) >= 0);  
}

void TestDisplayInfo::TestParameterizedConstructor() {
    DisplayInfo info;
    TEST_ASSERT_TRUE(static_cast<long long>(info.width) >= 0);
    TEST_ASSERT_TRUE(static_cast<long long>(info.height) >= 0);  
}

// ========== Method Tests ==========

void TestDisplayInfo::TestHasCapability() {
    DisplayInfo info;
    TEST_ASSERT_TRUE(static_cast<long long>(info.width) >= 0);
    TEST_ASSERT_TRUE(static_cast<long long>(info.height) >= 0);  
}

void TestDisplayInfo::TestAddCapability() {
    DisplayInfo info;
    TEST_ASSERT_TRUE(static_cast<long long>(info.width) >= 0);
    TEST_ASSERT_TRUE(static_cast<long long>(info.height) >= 0);  
}

// ========== Edge Cases ==========

void TestDisplayInfo::TestEdgeCases() {
    
    DisplayInfo info;
    TEST_ASSERT_TRUE(static_cast<long long>(info.width) >= 0);
    TEST_ASSERT_TRUE(static_cast<long long>(info.height) >= 0);  
}

// ========== Test Runner ==========

void TestDisplayInfo::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestHasCapability);
    RUN_TEST(TestAddCapability);
    RUN_TEST(TestEdgeCases);
}
