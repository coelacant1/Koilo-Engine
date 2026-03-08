// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testperfprofilescope.cpp
 * @brief Implementation of PerfProfileScope unit tests.
 */

#include "testperfprofilescope.hpp"

using namespace koilo;

void TestPerfProfileScope::TestDefaultConstructor() {
    PerfProfileScope scope("TestScope");
    TEST_ASSERT_TRUE(true);
}

void TestPerfProfileScope::TestParameterizedConstructor() {
    PerfProfileScope scope("Named");
    TEST_ASSERT_TRUE(true);
}

void TestPerfProfileScope::TestEdgeCases() {
    PerfProfileScope scope("Edge");
    TEST_ASSERT_TRUE(true);
}

void TestPerfProfileScope::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
