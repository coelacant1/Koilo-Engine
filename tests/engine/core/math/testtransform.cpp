// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtransform.cpp
 * @brief Implementation of Transform unit tests.
 */

#include "testtransform.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestTransform::TestDefaultConstructor() {
    Transform t;
    // Default transform should have identity values
    Vector3D pos = t.GetPosition();
    Vector3D scale = t.GetScale();
    
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(0, 0, 0), pos);
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(1, 1, 1), scale);
}

// ========== Position Tests ==========

void TestTransform::TestTranslate() {
    Transform t;
    t.SetPosition(Vector3D(10.0f, 20.0f, 30.0f));
    
    t.Translate(Vector3D(5.0f, 10.0f, 15.0f));
    
    Vector3D result = t.GetPosition();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, Vector3D(15.0f, 30.0f, 45.0f), result);
}

// ========== Rotation Tests ==========

void TestTransform::TestRotate() {
    Transform t;
    t.SetRotation(Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
    
    // Rotate by 90 degrees around X
    t.Rotate(Vector3D(90.0f, 0.0f, 0.0f));
    
    Quaternion result = t.GetRotation();
    // Verify rotation changed
    TEST_ASSERT_TRUE(result.W != 1.0f || result.X != 0.0f);
}

// ========== Scale Tests ==========

// ========== Offset Tests ==========

// ========== Integration Tests ==========

void TestTransform::TestToString() {
    Transform t;
    t.SetPosition(Vector3D(1.0f, 2.0f, 3.0f));
    
    koilo::UString str = t.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Test Runner ==========

void TestTransform::TestEdgeCases() {
    Transform zero;
    Vector3D zeroPos = zero.GetPosition();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, Vector3D(0, 0, 0), zeroPos);
    
    Transform large;
    large.SetPosition(Vector3D(1e6f, 1e6f, 1e6f));
    large.SetScale(Vector3D(1e6f, 1e6f, 1e6f));
    Vector3D largePos = large.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(1e3f, 1e6f, largePos.X);
    
    Transform negative;
    negative.SetPosition(Vector3D(-100.0f, -200.0f, -300.0f));
    Vector3D negPos = negative.GetPosition();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, Vector3D(-100.0f, -200.0f, -300.0f), negPos);
}

void TestTransform::TestGetBaseRotation() {
    Transform t;
    Quaternion baseRot(0.6f, 0.4f, 0.3f, 0.2f);
    t.SetBaseRotation(baseRot);
    
    Quaternion result = t.GetBaseRotation();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, baseRot.W, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, baseRot.X, result.X);
}

void TestTransform::TestGetPosition() {
    Transform t;
    Vector3D pos(5.0f, 10.0f, 15.0f);
    t.SetPosition(pos);
    
    Vector3D result = t.GetPosition();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, pos, result);
}

void TestTransform::TestGetRotation() {
    Transform t;
    Quaternion rot(0.707f, 0.707f, 0.0f, 0.0f);
    t.SetRotation(rot);
    
    Quaternion result = t.GetRotation();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, rot.W, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, rot.X, result.X);
}

void TestTransform::TestGetRotationOffset() {
    Transform t;
    Vector3D offset(1.0f, 2.0f, 3.0f);
    t.SetRotationOffset(offset);
    
    Vector3D result = t.GetRotationOffset();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, offset, result);
}

void TestTransform::TestGetScale() {
    Transform t;
    Vector3D scl(2.0f, 3.0f, 4.0f);
    t.SetScale(scl);
    
    Vector3D result = t.GetScale();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, scl, result);
}

void TestTransform::TestGetScaleOffset() {
    Transform t;
    Vector3D offset(0.5f, 1.0f, 1.5f);
    t.SetScaleOffset(offset);
    
    Vector3D result = t.GetScaleOffset();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, offset, result);
}

void TestTransform::TestParameterizedConstructor() {
    Vector3D pos(10.0f, 20.0f, 30.0f);
    Vector3D scl(2.0f, 3.0f, 4.0f);
    Quaternion rot(1.0f, 0.0f, 0.0f, 0.0f);
    
    Transform t(rot, pos, scl);
    
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, pos, t.GetPosition());
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, scl, t.GetScale());
}

void TestTransform::TestScale() {
    Transform t;
    t.SetScale(Vector3D(1.0f, 1.0f, 1.0f));
    
    t.Scale(Vector3D(2.0f, 3.0f, 4.0f));
    
    Vector3D result = t.GetScale();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, Vector3D(2.0f, 3.0f, 4.0f), result);
}

void TestTransform::TestSetBaseRotation() {
    Transform t;
    Quaternion baseRot(0.8f, 0.6f, 0.0f, 0.0f);
    
    t.SetBaseRotation(baseRot);
    
    Quaternion result = t.GetBaseRotation();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, baseRot.W, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, baseRot.X, result.X);
}

void TestTransform::TestSetPosition() {
    Transform t;
    Vector3D pos(100.0f, 200.0f, 300.0f);
    
    t.SetPosition(pos);
    
    Vector3D result = t.GetPosition();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, pos, result);
}

void TestTransform::TestSetRotation() {
    Transform t;
    Quaternion rot(0.5f, 0.5f, 0.5f, 0.5f);
    
    t.SetRotation(rot);
    
    Quaternion result = t.GetRotation();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, rot.W, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, rot.X, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, rot.Y, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, rot.Z, result.Z);
}

void TestTransform::TestSetRotationOffset() {
    Transform t;
    Vector3D offset(5.0f, 10.0f, 15.0f);
    
    t.SetRotationOffset(offset);
    
    Vector3D result = t.GetRotationOffset();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, offset, result);
}

void TestTransform::TestSetScale() {
    Transform t;
    Vector3D scl(5.0f, 6.0f, 7.0f);
    
    t.SetScale(scl);
    
    Vector3D result = t.GetScale();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, scl, result);
}

void TestTransform::TestSetScaleOffset() {
    Transform t;
    Vector3D offset(2.0f, 3.0f, 4.0f);
    
    t.SetScaleOffset(offset);
    
    Vector3D result = t.GetScaleOffset();
    TEST_ASSERT_VECTOR3D_WITHIN(0.01f, offset, result);
}

void TestTransform::TestGetOrigin() {
    // TODO: Implement test for GetOrigin()
    Transform obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestTransform::TestSetOrigin() {
    // TODO: Implement test for SetOrigin()
    Transform obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestTransform::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);

    RUN_TEST(TestTranslate);

    RUN_TEST(TestRotate);

    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetBaseRotation);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestGetRotation);
    RUN_TEST(TestGetRotationOffset);
    RUN_TEST(TestGetScale);
    RUN_TEST(TestGetScaleOffset);

    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestScale);
    RUN_TEST(TestSetBaseRotation);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestSetRotation);
    RUN_TEST(TestSetRotationOffset);
    RUN_TEST(TestSetScale);
    RUN_TEST(TestSetScaleOffset);

    RUN_TEST(TestGetOrigin);
    RUN_TEST(TestSetOrigin);
}
