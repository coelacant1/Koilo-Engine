// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeulerorder.cpp
 * @brief Implementation of EulerOrder unit tests.
 */

#include "testeulerorder.hpp"

using namespace koilo;
void TestEulerOrder::TestDefaultConstructor() {
    EulerOrder order;
    TEST_ASSERT_EQUAL_INT(EulerOrder::XYZ, order.AxisOrder);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Static, order.FrameTaken);
}

void TestEulerOrder::TestParameterizedConstructor() {
    Vector3D perm(0, 1, 2);
    EulerOrder order(EulerOrder::ZYX, EulerOrder::Rotating, perm);
    TEST_ASSERT_EQUAL_INT(EulerOrder::ZYX, order.AxisOrder);
    TEST_ASSERT_EQUAL_INT(EulerOrder::Rotating, order.FrameTaken);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, order.Permutation.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, order.Permutation.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, order.Permutation.Z);
}

void TestEulerOrder::TestToString() {
    EulerOrder order;
    koilo::UString str = order.ToString();
    TEST_ASSERT_NOT_NULL(str.CStr());
    TEST_ASSERT_TRUE(str.Length() > 0);
}

void TestEulerOrder::TestEdgeCases() {
    EulerOrder order1(EulerOrder::XYZ, EulerOrder::Static, Vector3D(0, 1, 2));
    EulerOrder order2(EulerOrder::ZYX, EulerOrder::Rotating, Vector3D(2, 1, 0));
    TEST_ASSERT_NOT_EQUAL(order1.AxisOrder, order2.AxisOrder);
    TEST_ASSERT_NOT_EQUAL(order1.FrameTaken, order2.FrameTaken);
}

void TestEulerOrder::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
}
