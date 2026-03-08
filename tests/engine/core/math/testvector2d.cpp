// SPDX-License-Identifier: GPL-3.0-or-later
#include "testvector2d.hpp"

using namespace koilo;
void TestVector2D::TestAbsolute() {
    Vector2D v(-3.0, -4.0);
    Vector2D absV = v.Absolute();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, absV.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, absV.Y);
}

void TestVector2D::TestNormal() {
    Vector2D v(3.0, 4.0);
    Vector2D normV = v.Normal();
    float magnitude = sqrt(normV.X * normV.X + normV.Y * normV.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, magnitude);
}

void TestVector2D::TestAdd() {
    Vector2D v1(1.0, 2.0);
    Vector2D v2(3.0, 4.0);
    Vector2D result = v1.Add(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 6.0, result.Y);
}

void TestVector2D::TestSubtract() {
    Vector2D v1(5.0, 6.0);
    Vector2D v2(3.0, 4.0);
    Vector2D result = v1.Subtract(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.Y);
}

void TestVector2D::TestCrossProduct() {
    Vector2D v1(1.0, 2.0);
    Vector2D v2(3.0, 4.0);
    float result = v1.CrossProduct(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -2.0, result);
}

void TestVector2D::TestUnitCircle() {
    Vector2D v = Vector2D().UnitCircle();
    float magnitude = sqrt(v.X * v.X + v.Y * v.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, magnitude);
}

void TestVector2D::TestMinimum() {
    Vector2D v1(1.0, 4.0);
    Vector2D v2(2.0, 3.0);
    Vector2D result = v1.Minimum(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, result.Y);
}

void TestVector2D::TestMaximum() {
    Vector2D v1(1.0, 4.0);
    Vector2D v2(2.0, 3.0);
    Vector2D result = v1.Maximum(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, result.Y);
}

void TestVector2D::TestRotate() {
    Vector2D v(1.0, 0.0);
    Vector2D offset(0.0, 0.0);
    Vector2D result = v.Rotate(90.0, offset);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, result.Y);
}

void TestVector2D::TestCheckBounds() {
    Vector2D v(5.0, 5.0);
    Vector2D min(1.0, 1.0);
    Vector2D max(10.0, 10.0);
    TEST_ASSERT_TRUE(v.CheckBounds(min, max));

    Vector2D outOfBounds(11.0, 5.0);
    TEST_ASSERT_FALSE(outOfBounds.CheckBounds(min, max));
}

void TestVector2D::TestMagnitude() {
    Vector2D v(3.0, 4.0);
    float result = v.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, result);
}

void TestVector2D::TestDotProduct() {
    Vector2D v1(1.0, 2.0);
    Vector2D v2(3.0, 4.0);
    float result = v1.DotProduct(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 11.0, result);
}

void TestVector2D::TestCalculateEuclideanDistance() {
    Vector2D v1(1.0, 1.0);
    Vector2D v2(4.0, 5.0);
    float result = v1.CalculateEuclideanDistance(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, result);
}

void TestVector2D::TestIsEqual() {
    Vector2D v1(1.0, 2.0);
    Vector2D v2(1.0, 2.0);
    Vector2D v3(2.0, 3.0);
    TEST_ASSERT_TRUE(v1.IsEqual(v2));
    TEST_ASSERT_FALSE(v1.IsEqual(v3));
}

void TestVector2D::TestToString() {
    Vector2D v(1.0, 2.0);
    koilo::UString str = v.ToString();
    TEST_ASSERT_EQUAL_STRING("[1.000, 2.000]", str.CStr());
}

void TestVector2D::TestDefaultConstructor() {
    Vector2D v;
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.Y);
}

void TestVector2D::TestDivide() {
    Vector2D v1(8.0, 15.0);
    Vector2D v2(2.0, 3.0);
    Vector2D result = v1.Divide(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, result.Y);
    
    Vector2D scalar_result = v1.Divide(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, scalar_result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.5, scalar_result.Y);
}

void TestVector2D::TestEdgeCases() {
    Vector2D zero(0.0, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, zero.Magnitude());
    
    Vector2D normalized_zero = zero.UnitCircle();
    TEST_ASSERT_FALSE(isnan(normalized_zero.X) && isnan(normalized_zero.Y));
    
    Vector2D large(1e6, 1e6);
    Vector2D large_add = large.Add(Vector2D(1.0, 1.0));
    TEST_ASSERT_FLOAT_WITHIN(1.0, 1e6 + 1.0, large_add.X);
    
    Vector2D negative(-5.0, -10.0);
    Vector2D abs_neg = negative.Absolute();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, abs_neg.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10.0, abs_neg.Y);
}

void TestVector2D::TestMultiply() {
    Vector2D v1(2.0, 3.0);
    Vector2D v2(4.0, 5.0);
    Vector2D result = v1.Multiply(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 8.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 15.0, result.Y);
    
    Vector2D scalar_result = v1.Multiply(2.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, scalar_result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.5, scalar_result.Y);
}

void TestVector2D::TestParameterizedConstructor() {
    Vector2D v(3.5, 7.2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.5, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.2, v.Y);
    
    Vector2D copy(v);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.5, copy.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.2, copy.Y);
}

void TestVector2D::TestPerpendicular() {
    Vector2D v(1.0, 0.0);
    Vector2D perp = v.Perpendicular();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, perp.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, perp.Y);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.DotProduct(perp));
}

void TestVector2D::TestRightPerpendicular() {
    Vector2D v(1.0, 0.0);
    Vector2D perp = v.RightPerpendicular();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, perp.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -1.0, perp.Y);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.DotProduct(perp));
}

void TestVector2D::RunAllTests() {

    RUN_TEST(TestAbsolute);
    RUN_TEST(TestNormal);
    RUN_TEST(TestAdd);
    RUN_TEST(TestSubtract);

    RUN_TEST(TestCrossProduct);
    RUN_TEST(TestUnitCircle);

    RUN_TEST(TestMinimum);
    RUN_TEST(TestMaximum);
    RUN_TEST(TestRotate);
    RUN_TEST(TestCheckBounds);
    RUN_TEST(TestMagnitude);
    RUN_TEST(TestDotProduct);
    RUN_TEST(TestCalculateEuclideanDistance);
    RUN_TEST(TestIsEqual);
    RUN_TEST(TestToString);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestDivide);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestMultiply);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPerpendicular);
    RUN_TEST(TestRightPerpendicular);
}
