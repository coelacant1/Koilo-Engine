// SPDX-License-Identifier: GPL-3.0-or-later
#include "testvector3d.hpp"
#include <cmath>

using namespace koilo;
void TestVector3D::TestAbsolute() {
    Vector3D v(-3.0, -4.0, -5.0);
    Vector3D absV = v.Absolute();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, absV.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, absV.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, absV.Z);
}

void TestVector3D::TestNormal() {
    Vector3D v(3.0, 4.0, 5.0);
    Vector3D normV = v.Normal();
    float magnitude = sqrt(normV.X * normV.X + normV.Y * normV.Y + normV.Z * normV.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, magnitude);
}

void TestVector3D::TestAdd() {
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(3.0, 4.0, 5.0);
    Vector3D result = v1.Add(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 6.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 8.0, result.Z);
}

void TestVector3D::TestSubtract() {
    Vector3D v1(5.0, 6.0, 7.0);
    Vector3D v2(3.0, 4.0, 5.0);
    Vector3D result = v1.Subtract(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.Z);
}

void TestVector3D::TestCrossProduct() {
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(4.0, 5.0, 6.0);
    Vector3D result = v1.CrossProduct(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -3.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 6.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -3.0, result.Z);
}

void TestVector3D::TestUnitSphere() {
    Vector3D v = Vector3D().UnitSphere();
    float magnitude = sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, magnitude);
}

void TestVector3D::TestMagnitude() {
    Vector3D v(3.0, 4.0, 5.0);
    float result = v.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01, sqrt(50.0), result);
}

void TestVector3D::TestDotProduct() {
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(4.0, 5.0, 6.0);
    float result = v1.DotProduct(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 32.0, result);
}

void TestVector3D::TestCalculateEuclideanDistance() {
    Vector3D v1(1.0, 1.0, 1.0);
    Vector3D v2(4.0, 5.0, 6.0);
    float result = v1.CalculateEuclideanDistance(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, sqrt(50.0), result);
}

void TestVector3D::TestIsEqual() {
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(1.0, 2.0, 3.0);
    Vector3D v3(2.0, 3.0, 4.0);
    TEST_ASSERT_TRUE(v1.IsEqual(v2));
    TEST_ASSERT_FALSE(v1.IsEqual(v3));
}

void TestVector3D::TestToString() {
    Vector3D v(1.0, 2.0, 3.0);
    koilo::UString str = v.ToString();
    TEST_ASSERT_EQUAL_STRING("[1.000, 2.000, 3.000]", str.CStr());
}

void TestVector3D::TestAverageHighestTwoComponents() {
    Vector3D v(1.0, 5.0, 3.0);
    float avg = v.AverageHighestTwoComponents();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, avg);
    
    Vector3D v2(10.0, 2.0, 8.0);
    float avg2 = v2.AverageHighestTwoComponents();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 9.0, avg2);
}

void TestVector3D::TestDefaultConstructor() {
    Vector3D v;
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, v.Z);
}

void TestVector3D::TestDivide() {
    Vector3D v1(8.0, 15.0, 20.0);
    Vector3D v2(2.0, 3.0, 4.0);
    Vector3D result = v1.Divide(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, result.Z);
    
    Vector3D scalar_result = v1.Divide(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, scalar_result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.5, scalar_result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10.0, scalar_result.Z);
}

void TestVector3D::TestEdgeCases() {
    Vector3D zero(0.0, 0.0, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, zero.Magnitude());
    
    Vector3D normalized_zero = zero.UnitSphere();
    TEST_ASSERT_FALSE(std::isnan(normalized_zero.X) && std::isnan(normalized_zero.Y) && std::isnan(normalized_zero.Z));
    
    Vector3D large(1e6, 1e6, 1e6);
    Vector3D large_add = large.Add(Vector3D(1.0, 1.0, 1.0));
    TEST_ASSERT_FLOAT_WITHIN(1.0, 1e6 + 1.0, large_add.X);
    
    Vector3D negative(-5.0, -10.0, -15.0);
    Vector3D abs_neg = negative.Absolute();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, abs_neg.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10.0, abs_neg.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 15.0, abs_neg.Z);
}

void TestVector3D::TestMax() {
    Vector3D v(3.0, 1.5, 7.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.0, v.Max());
    
    Vector3D negative(-5.0, -2.0, -10.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -2.0, negative.Max());
}

void TestVector3D::TestMin() {
    Vector3D v(3.0, 1.5, 7.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.5, v.Min());
    
    Vector3D negative(-5.0, -2.0, -10.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -10.0, negative.Min());
}

void TestVector3D::TestMultiply() {
    Vector3D v1(2.0, 3.0, 4.0);
    Vector3D v2(4.0, 5.0, 6.0);
    Vector3D result = v1.Multiply(v2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 8.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 15.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 24.0, result.Z);
    
    Vector3D scalar_result = v1.Multiply(2.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, scalar_result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.5, scalar_result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10.0, scalar_result.Z);
}

void TestVector3D::TestParameterizedConstructor() {
    Vector3D v(3.5, 7.2, 9.1);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.5, v.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.2, v.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 9.1, v.Z);
    
    Vector3D copy(v);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.5, copy.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 7.2, copy.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 9.1, copy.Z);
}

void TestVector3D::TestPermutate() {
    Vector3D v(1.0, 2.0, 3.0);
    Vector3D perm(0.0, 2.0, 1.0);
    Vector3D result = v.Permutate(perm);
    TEST_ASSERT_FALSE(std::isnan(result.X) && std::isnan(result.Y) && std::isnan(result.Z));
}

void TestVector3D::RunAllTests() {

    RUN_TEST(TestAbsolute);
    RUN_TEST(TestNormal);
    RUN_TEST(TestAdd);
    RUN_TEST(TestSubtract);

    RUN_TEST(TestCrossProduct);
    RUN_TEST(TestUnitSphere);

    RUN_TEST(TestMagnitude);
    RUN_TEST(TestDotProduct);
    RUN_TEST(TestCalculateEuclideanDistance);
    RUN_TEST(TestIsEqual);
    RUN_TEST(TestToString);
    RUN_TEST(TestAverageHighestTwoComponents);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestDivide);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestMax);
    RUN_TEST(TestMin);
    RUN_TEST(TestMultiply);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPermutate);
}
