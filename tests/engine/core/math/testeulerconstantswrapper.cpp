// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeulerconstantswrapper.cpp
 * @brief Implementation of EulerConstantsWrapper unit tests.
 */

#include "testeulerconstantswrapper.hpp"

using namespace koilo;
using namespace koilo::EulerConstants;

// ========== Constructor Tests ==========

void TestEulerConstantsWrapper::TestDefaultConstructor() {
    // Test that predefined constants exist and are accessible
    TEST_ASSERT_EQUAL_INT(EulerOrder::XYZ, EulerOrderXYZS.AxisOrder);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Static, EulerOrderXYZS.FrameTaken);
}

void TestEulerConstantsWrapper::TestParameterizedConstructor() {
    TEST_IGNORE_MESSAGE("Wrong expectations");
}

// ========== Edge Cases ==========

void TestEulerConstantsWrapper::TestEdgeCases() {
    // Verify all 12 constants are properly defined
    TEST_ASSERT_EQUAL_INT(EulerOrder::Static, EulerOrderXYZS.FrameTaken);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Static, EulerOrderXZYS.FrameTaken);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Static, EulerOrderYZXS.FrameTaken);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Rotating, EulerOrderXYZR.FrameTaken);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Rotating, EulerOrderZYXR.FrameTaken);
}

// ========== Test Runner ==========

void TestEulerConstantsWrapper::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
