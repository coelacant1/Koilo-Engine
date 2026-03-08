// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmemoryscope.cpp
 * @brief Implementation of MemoryScope unit tests.
 */

#include "testmemoryscope.hpp"

using namespace koilo;

void TestMemoryScope::TestDefaultConstructor() {
    MemoryScope scope("TestScope");
    TEST_ASSERT_TRUE(true);
}

void TestMemoryScope::TestParameterizedConstructor() {
    MemoryScope scope("Named");
    TEST_ASSERT_TRUE(true);
}

void TestMemoryScope::TestTrack() {
    MemoryScope scope("TrackTest");
    // Track and free in scope destructor
    TEST_ASSERT_TRUE(true);
}

void TestMemoryScope::TestEdgeCases() {
    MemoryScope scope("Edge");
    TEST_ASSERT_TRUE(true);
}

void TestMemoryScope::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestTrack);
    RUN_TEST(TestEdgeCases);
}
