// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testimaterial.cpp
 * @brief Implementation of IMaterial unit tests.
 */

#include "testimaterial.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestIMaterial::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // IMaterial cannot be default-constructed
    TEST_IGNORE_MESSAGE("Stub");
}

void TestIMaterial::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Method Tests ==========

void TestIMaterial::TestGetShader() {
    // TODO: Implement test for GetShader()
    // IMaterial cannot be default-constructed
    TEST_IGNORE_MESSAGE("Stub");
}

void TestIMaterial::TestUpdate() {
    // TODO: Implement test for Update()
    // IMaterial cannot be default-constructed
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Edge Cases ==========

void TestIMaterial::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Test Runner ==========

void TestIMaterial::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetShader);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestEdgeCases);
}
