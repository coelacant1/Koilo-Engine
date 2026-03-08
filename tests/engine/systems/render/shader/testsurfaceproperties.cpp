// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsurfaceproperties.cpp
 * @brief Implementation of SurfaceProperties unit tests.
 */

#include "testsurfaceproperties.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestSurfaceProperties::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // SurfaceProperties cannot be default-constructed
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSurfaceProperties::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Edge Cases ==========

void TestSurfaceProperties::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Test Runner ==========

void TestSurfaceProperties::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
