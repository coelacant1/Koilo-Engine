#include "testrotation.hpp"

void TestRotation::TestEulerAngleConversionXYZ(Vector3D xyz, Quaternion q){
    EulerAngles ea(xyz, EulerConstants::EulerOrderXYZS);
    Rotation r(ea);
    Quaternion qOut = r.GetQuaternion();
    TEST_ASSERT_EQUAL_FLOAT(q.W, qOut.W);
    TEST_ASSERT_EQUAL_FLOAT(q.X, qOut.X);
    TEST_ASSERT_EQUAL_FLOAT(q.Y, qOut.Y);
    TEST_ASSERT_EQUAL_FLOAT(q.Z, qOut.Z);
}

void TestRotation::TestDefaultConstructor() {
    Rotation r;
    Quaternion q = r.GetQuaternion();
    TEST_ASSERT_EQUAL_FLOAT(1.0f, q.W);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, q.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, q.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, q.Z);
}

void TestRotation::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestGetAxisAngle() {
    // TODO: Implement test for GetAxisAngle()
    // Rotation obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestGetDirectionAngle() {
    // TODO: Implement test for GetDirectionAngle()
    // Rotation obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestGetEulerAngles() {
    // TODO: Implement test for GetEulerAngles()
    // Rotation obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestGetQuaternion() {
    // TODO: Implement test for GetQuaternion()
    // Rotation obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestGetRotationMatrix() {
    // TODO: Implement test for GetRotationMatrix()
    // Rotation obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestGetYawPitchRoll() {
    // TODO: Implement test for GetYawPitchRoll()
    // Rotation obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRotation::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);

    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetAxisAngle);
    RUN_TEST(TestGetDirectionAngle);
    RUN_TEST(TestGetEulerAngles);
    RUN_TEST(TestGetQuaternion);
    RUN_TEST(TestGetRotationMatrix);
    RUN_TEST(TestGetYawPitchRoll);
    RUN_TEST(TestParameterizedConstructor);
}
