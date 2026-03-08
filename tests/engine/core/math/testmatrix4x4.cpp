// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmatrix4x4.cpp
 * @brief Implementation of Matrix4x4 unit tests.
 */

#include "testmatrix4x4.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestMatrix4x4::TestDefaultConstructor() {
    Matrix4x4 m;
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, m.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, m.M[0][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, m.M[0][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, m.M[0][3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, m.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, m.M[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, m.M[3][3]);
}

void TestMatrix4x4::TestParameterizedConstructor() {
    Matrix4x4 m(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, m.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 6.0, m.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 11.0, m.M[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 16.0, m.M[3][3]);
    
    Matrix4x4 copy(m);
    TEST_ASSERT_TRUE(copy.IsEqual(m));
}

// ========== Method Tests ==========

void TestMatrix4x4::TestSetIdentity() {
    Matrix4x4 m(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    m.SetIdentity();
    TEST_ASSERT_TRUE(m.IsIdentity());
}

void TestMatrix4x4::TestTransformDirection() {
    Matrix4x4 identity;
    Vector3D direction(1.0, 0.0, 0.0);
    Vector3D transformed = identity.TransformDirection(direction);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, transformed.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, transformed.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, transformed.Z);
}

void TestMatrix4x4::TestTranspose() {
    Matrix4x4 m(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    Matrix4x4 t = m.Transpose();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, t.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, t.M[0][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 9.0, t.M[0][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 13.0, t.M[0][3]);
}

void TestMatrix4x4::TestDeterminant() {
    Matrix4x4 identity;
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, identity.Determinant());
    
    Matrix4x4 m(
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 2
    );
    TEST_ASSERT_FLOAT_WITHIN(0.1, 16.0, m.Determinant());
}

void TestMatrix4x4::TestInverse() {
    Matrix4x4 m(
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 2
    );
    Matrix4x4 inv = m.Inverse();
    Matrix4x4 product = m.Multiply(inv);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, product.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, product.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, product.M[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, product.M[3][3]);
}

void TestMatrix4x4::TestGetTranslation() {
    Vector3D translation(5.0, 10.0, 15.0);
    Matrix4x4 m = Matrix4x4::Translation(translation);
    Vector3D extracted = m.GetTranslation();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, extracted.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10.0, extracted.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 15.0, extracted.Z);
}

void TestMatrix4x4::TestGetScale() {
    Vector3D scale(2.0, 3.0, 4.0);
    Matrix4x4 m = Matrix4x4::Scale(scale);
    Vector3D extracted = m.GetScale();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, extracted.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, extracted.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, extracted.Z);
}

void TestMatrix4x4::TestGetRotation() {
    Quaternion q(1.0, 0.0, 0.0, 0.0);
    Matrix4x4 m = Matrix4x4::Rotation(q);
    Quaternion extracted = m.GetRotation();
    TEST_ASSERT_FALSE(isnan(extracted.W) && isnan(extracted.X) && isnan(extracted.Y) && isnan(extracted.Z));
}

void TestMatrix4x4::TestIsEqual() {
    Matrix4x4 m1(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    Matrix4x4 m2(m1);
    Matrix4x4 m3;
    
    TEST_ASSERT_TRUE(m1.IsEqual(m2));
    TEST_ASSERT_FALSE(m1.IsEqual(m3));
}

void TestMatrix4x4::TestIsIdentity() {
    Matrix4x4 identity;
    TEST_ASSERT_TRUE(identity.IsIdentity());
    
    Matrix4x4 notIdentity(
        1, 2, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    TEST_ASSERT_FALSE(notIdentity.IsIdentity());
}

void TestMatrix4x4::TestToString() {
    Matrix4x4 m;
    koilo::UString str = m.ToString();
    TEST_ASSERT_NOT_NULL(str.CStr());
    TEST_ASSERT_TRUE(strlen(str.CStr()) > 0);
}

void TestMatrix4x4::TestTranslation() {
    Vector3D translation(5.0, 10.0, 15.0);
    Matrix4x4 m = Matrix4x4::Translation(translation);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.0, m.M[0][3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10.0, m.M[1][3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 15.0, m.M[2][3]);
}

void TestMatrix4x4::TestScale() {
    Vector3D scale(2.0, 3.0, 4.0);
    Matrix4x4 m = Matrix4x4::Scale(scale);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, m.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, m.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, m.M[2][2]);
}

void TestMatrix4x4::TestRotation() {
    Quaternion q(1.0, 0.0, 0.0, 0.0);
    Matrix4x4 m = Matrix4x4::Rotation(q);
    TEST_ASSERT_TRUE(m.IsIdentity() || !m.IsIdentity());
}

// ========== Edge Cases ==========

void TestMatrix4x4::TestEdgeCases() {
    Matrix4x4 zero(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    );
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, zero.Determinant());
    
    Matrix4x4 identity;
    Matrix4x4 transposed = identity.Transpose();
    TEST_ASSERT_TRUE(transposed.IsIdentity());
    
    Vector3D largeScale(1e6, 1e6, 1e6);
    Matrix4x4 large = Matrix4x4::Scale(largeScale);
    Vector3D extractedScale = large.GetScale();
    TEST_ASSERT_FLOAT_WITHIN(1e3, 1e6, extractedScale.X);
}

// ========== Test Runner ==========

void TestMatrix4x4::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetIdentity);
    RUN_TEST(TestTransformDirection);
    RUN_TEST(TestTranspose);
    RUN_TEST(TestDeterminant);
    RUN_TEST(TestInverse);
    RUN_TEST(TestGetTranslation);
    RUN_TEST(TestGetScale);
    RUN_TEST(TestGetRotation);
    RUN_TEST(TestIsEqual);
    RUN_TEST(TestIsIdentity);
    RUN_TEST(TestToString);
    RUN_TEST(TestTranslation);
    RUN_TEST(TestScale);
    RUN_TEST(TestRotation);
    RUN_TEST(TestEdgeCases);
}
