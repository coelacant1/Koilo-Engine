// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeulerangles.cpp
 * @brief Implementation of EulerAngles unit tests.
 */

#include "testeulerangles.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestEulerAngles::TestDefaultConstructor() {
    EulerAngles euler;

    // Default should have zero angles
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), euler.Angles);
}

void TestEulerAngles::TestParameterizedConstructor() {
    Vector3D angles(45.0f, 30.0f, 60.0f);
    EulerOrder order(EulerOrder::XYZ, EulerOrder::Static, Vector3D(0, 1, 2));

    EulerAngles euler(angles, order);

    TEST_ASSERT_VECTOR3D_EQUAL(angles, euler.Angles);
    TEST_ASSERT_EQUAL(EulerOrder::XYZ, euler.Order.AxisOrder);
}

// ========== Field Access Tests ==========

// ========== String Conversion Tests ==========

void TestEulerAngles::TestToString() {
    Vector3D angles(45.0f, 30.0f, 60.0f);
    EulerOrder order(EulerOrder::XYZ, EulerOrder::Static, Vector3D(0, 1, 2));
    EulerAngles euler(angles, order);

    koilo::UString str = euler.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Rotation Order Tests ==========

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestEulerAngles::TestEdgeCases() {
    EulerAngles zero;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Angles.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Angles.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Angles.Z);
    
    Vector3D large(360.0f, 360.0f, 360.0f);
    EulerOrder order(EulerOrder::XYZ, EulerOrder::Static, Vector3D(0, 1, 2));
    EulerAngles euler(large, order);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 360.0f, euler.Angles.X);
}

void TestEulerAngles::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestToString);

    RUN_TEST(TestEdgeCases);
}
