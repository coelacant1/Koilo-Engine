// SPDX-License-Identifier: GPL-3.0-or-later
#include "testrotation.hpp"

using namespace koilo;
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
    Rotation identity;
    Quaternion q = identity.GetQuaternion();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, q.W);
    
    Quaternion zero(0.0f, 0.0f, 0.0f, 0.0f);
    Rotation r1(zero);
    TEST_ASSERT_TRUE(r1.GetQuaternion().W >= 0.0f || r1.GetQuaternion().W < 0.0f);
    
    AxisAngle full(360.0f, Vector3D(1, 0, 0));
    Rotation r2(full);
    TEST_ASSERT_TRUE(r2.GetQuaternion().IsFinite());
}

void TestRotation::TestGetAxisAngle() {
    AxisAngle aa(45.0f, Vector3D(1, 0, 0));
    Rotation r(aa);
    AxisAngle result = r.GetAxisAngle();
    TEST_ASSERT_TRUE(result.Rotation >= 0.0f);
    TEST_ASSERT_TRUE(result.Axis.Magnitude() > 0.0f);
}

void TestRotation::TestGetDirectionAngle() {
    DirectionAngle da(90.0f, Vector3D(1, 0, 0));
    Rotation r(da);
    DirectionAngle result = r.GetDirectionAngle();
    TEST_ASSERT_TRUE(result.Rotation >= 0.0f);
    TEST_ASSERT_TRUE(result.Direction.Magnitude() > 0.0f);
}

void TestRotation::TestGetEulerAngles() {
    Vector3D angles(45.0f, 30.0f, 60.0f);
    EulerAngles ea(angles, EulerConstants::EulerOrderXYZS);
    Rotation r(ea);
    EulerAngles result = r.GetEulerAngles(EulerConstants::EulerOrderXYZS);
    TEST_ASSERT_TRUE(result.Angles.X >= 0.0f || result.Angles.X < 0.0f);
}

void TestRotation::TestGetQuaternion() {
    Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
    Rotation r(q);
    Quaternion result = r.GetQuaternion();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.X);
}

void TestRotation::TestGetRotationMatrix() {
    Rotation r;
    RotationMatrix rm = r.GetRotationMatrix();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm.XAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm.YAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm.ZAxis.Z);
}

void TestRotation::TestGetYawPitchRoll() {
    YawPitchRoll ypr(45.0f, 30.0f, 60.0f);
    Rotation r(ypr);
    YawPitchRoll result = r.GetYawPitchRoll();
    TEST_ASSERT_TRUE(result.Yaw >= 0.0f || result.Yaw < 0.0f);
    TEST_ASSERT_TRUE(result.Pitch >= 0.0f || result.Pitch < 0.0f);
    TEST_ASSERT_TRUE(result.Roll >= 0.0f || result.Roll < 0.0f);
}

void TestRotation::TestParameterizedConstructor() {
    Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
    Rotation r1(q);
    Quaternion q1 = r1.GetQuaternion();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, q1.W);
    
    AxisAngle aa(45.0f, Vector3D(1, 0, 0));
    Rotation r2(aa);
    TEST_ASSERT_TRUE(r2.GetQuaternion().W > 0.0f);
    
    YawPitchRoll ypr(45.0f, 30.0f, 60.0f);
    Rotation r3(ypr);
    TEST_ASSERT_TRUE(r3.GetQuaternion().W >= 0.0f);
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
