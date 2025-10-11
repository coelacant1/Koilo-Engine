#include "testvector3d.hpp"

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
    ptx::UString str = v.ToString();
    TEST_ASSERT_EQUAL_STRING("[1.000, 2.000, 3.000]", str.CStr());
}

void TestVector3D::TestAverageHighestTwoComponents() {
    // TODO: Implement test for AverageHighestTwoComponents()
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestDivide() {
    // TODO: Implement test for Divide()
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestMax() {
    // TODO: Implement test for Max()
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestMin() {
    // TODO: Implement test for Min()
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestMultiply() {
    // TODO: Implement test for Multiply()
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestVector3D::TestPermutate() {
    // TODO: Implement test for Permutate()
    // Vector3D obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
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
