#include "testquaternion.hpp"

void TestQuaternion::TestRotateVector(Quaternion q, Vector3D v, Vector3D e){
    Vector3D result = q.RotateVector(v);

    TEST_ASSERT_FLOAT_WITHIN(0.01, e.X, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, e.Y, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, e.Z, result.Z);
}

void TestQuaternion::TestAbsolute() {
    // TODO: Implement test for Absolute()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestAdd() {
    // TODO: Implement test for Add()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestAdditiveInverse() {
    // TODO: Implement test for AdditiveInverse()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestConjugate() {
    // TODO: Implement test for Conjugate()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestDeltaRotation() {
    // TODO: Implement test for DeltaRotation()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestDivide() {
    // TODO: Implement test for Divide()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestDotProduct() {
    // TODO: Implement test for DotProduct()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestGetBiVector() {
    // TODO: Implement test for GetBiVector()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestGetNormal() {
    // TODO: Implement test for GetNormal()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestIsClose() {
    // TODO: Implement test for IsClose()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestIsEqual() {
    // TODO: Implement test for IsEqual()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestIsFinite() {
    // TODO: Implement test for IsFinite()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestIsInfinite() {
    // TODO: Implement test for IsInfinite()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestIsNaN() {
    // TODO: Implement test for IsNaN()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestIsNonZero() {
    // TODO: Implement test for IsNonZero()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestMagnitude() {
    // TODO: Implement test for Magnitude()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestMultiplicativeInverse() {
    // TODO: Implement test for MultiplicativeInverse()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestMultiply() {
    // TODO: Implement test for Multiply()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestNormal() {
    // TODO: Implement test for Normal()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestPermutate() {
    // TODO: Implement test for Permutate()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestPower() {
    // TODO: Implement test for Power()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestRotateVectorUnit() {
    // TODO: Implement test for RotateVectorUnit()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestSubtract() {
    // TODO: Implement test for Subtract()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestToString() {
    // TODO: Implement test for ToString()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::TestUnitQuaternion() {
    // TODO: Implement test for UnitQuaternion()
    // Quaternion obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestQuaternion::RunAllTests() {

    RUN_TEST(TestAbsolute);
    RUN_TEST(TestAdd);
    RUN_TEST(TestAdditiveInverse);
    RUN_TEST(TestConjugate);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestDeltaRotation);
    RUN_TEST(TestDivide);
    RUN_TEST(TestDotProduct);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetBiVector);
    RUN_TEST(TestGetNormal);
    RUN_TEST(TestIsClose);
    RUN_TEST(TestIsEqual);
    RUN_TEST(TestIsFinite);
    RUN_TEST(TestIsInfinite);
    RUN_TEST(TestIsNaN);
    RUN_TEST(TestIsNonZero);
    RUN_TEST(TestMagnitude);
    RUN_TEST(TestMultiplicativeInverse);
    RUN_TEST(TestMultiply);
    RUN_TEST(TestNormal);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPermutate);
    RUN_TEST(TestPower);
    RUN_TEST(TestRotateVectorUnit);
    RUN_TEST(TestSubtract);
    RUN_TEST(TestToString);
    RUN_TEST(TestUnitQuaternion);
}
