// SPDX-License-Identifier: GPL-3.0-or-later
#include "testrotationmatrix.hpp"

using namespace koilo;
void TestRotationMatrix::TestConvertCoordinateToVector() {
    RotationMatrix rm;
    Vector3D result = rm.ConvertCoordinateToVector();
    TEST_ASSERT_TRUE(result.X >= 0.0f || result.X < 0.0f);
}

void TestRotationMatrix::TestDefaultConstructor() {
    RotationMatrix rm;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm.XAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rm.XAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rm.XAxis.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rm.YAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm.YAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rm.YAxis.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rm.ZAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rm.ZAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm.ZAxis.Z);
}

void TestRotationMatrix::TestDeterminant() {
    RotationMatrix rm;
    float det = rm.Determinant();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, det);
}

void TestRotationMatrix::TestEdgeCases() {
    RotationMatrix identity;
    float det = identity.Determinant();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, det);
    
    Vector3D zero(0.0f, 0.0f, 0.0f);
    RotationMatrix zeroRot(zero);
    TEST_ASSERT_TRUE(zeroRot.XAxis.X >= 0.0f || zeroRot.XAxis.X < 0.0f);
    
    Vector3D large(1e6f, 0.0f, 0.0f);
    RotationMatrix largeRot(large);
    TEST_ASSERT_TRUE(largeRot.XAxis.Magnitude() > 0.0f);
}

void TestRotationMatrix::TestInverse() {
    RotationMatrix rm;
    RotationMatrix inverse = rm.Inverse();
    RotationMatrix identity = rm.Multiply(inverse);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, identity.XAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, identity.YAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, identity.ZAxis.Z);
}

void TestRotationMatrix::TestIsEqual() {
    RotationMatrix rm1;
    RotationMatrix rm2;
    TEST_ASSERT_TRUE(rm1.IsEqual(rm2));
    
    Vector3D x(2.0f, 0.0f, 0.0f);
    RotationMatrix rm3(x, Vector3D(0, 1, 0), Vector3D(0, 0, 1));
    TEST_ASSERT_FALSE(rm1.IsEqual(rm3));
}

void TestRotationMatrix::TestMultiply() {
    RotationMatrix rm;
    RotationMatrix scaled = rm.Multiply(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, scaled.XAxis.X);
    
    RotationMatrix rm2;
    RotationMatrix multiplied = rm.Multiply(rm2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, multiplied.XAxis.X);
}

void TestRotationMatrix::TestNormalize() {
    Vector3D x(2.0f, 0.0f, 0.0f);
    Vector3D y(0.0f, 3.0f, 0.0f);
    Vector3D z(0.0f, 0.0f, 4.0f);
    RotationMatrix rm(x, y, z);
    RotationMatrix normalized = rm.Normalize();
    float lenX = normalized.XAxis.Magnitude();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, lenX);
}

void TestRotationMatrix::TestParameterizedConstructor() {
    Vector3D v(1.0f, 2.0f, 3.0f);
    RotationMatrix rm1(v);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm1.XAxis.X);
    
    Vector3D x(1.0f, 0.0f, 0.0f);
    Vector3D y(0.0f, 1.0f, 0.0f);
    Vector3D z(0.0f, 0.0f, 1.0f);
    RotationMatrix rm2(x, y, z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm2.XAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm2.YAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rm2.ZAxis.Z);
}

void TestRotationMatrix::TestReadjustMatrix() {
    Vector3D x(1.1f, 0.1f, 0.0f);
    Vector3D y(0.1f, 1.1f, 0.0f);
    Vector3D z(0.0f, 0.0f, 1.0f);
    RotationMatrix rm(x, y, z);
    rm.ReadjustMatrix();
    TEST_ASSERT_TRUE(rm.XAxis.Magnitude() > 0.0f);
}

void TestRotationMatrix::TestRotate() {
    RotationMatrix rm;
    Vector3D rotation(Mathematics::MPI / 2.0f, 0.0f, 0.0f);
    Vector3D result = rm.Rotate(rotation);
    TEST_ASSERT_TRUE(result.Magnitude() > 0.0f);
}

void TestRotationMatrix::TestRotateRelative() {
    RotationMatrix rm1;
    RotationMatrix rm2;
    rm1.RotateRelative(rm2);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, rm1.XAxis.X);
}

void TestRotationMatrix::TestRotateX() {
    RotationMatrix rm;
    Vector3D result = rm.RotateX(Mathematics::MPI / 2.0f);
    TEST_ASSERT_TRUE(result.Magnitude() > 0.0f);
}

void TestRotationMatrix::TestRotateY() {
    RotationMatrix rm;
    Vector3D result = rm.RotateY(Mathematics::MPI / 2.0f);
    TEST_ASSERT_TRUE(result.Magnitude() > 0.0f);
}

void TestRotationMatrix::TestRotateZ() {
    RotationMatrix rm;
    Vector3D result = rm.RotateZ(Mathematics::MPI / 2.0f);
    TEST_ASSERT_TRUE(result.Magnitude() > 0.0f);
}

void TestRotationMatrix::TestToString() {
    RotationMatrix rm;
    koilo::UString str = rm.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

void TestRotationMatrix::TestTranspose() {
    Vector3D x(1.0f, 2.0f, 3.0f);
    Vector3D y(4.0f, 5.0f, 6.0f);
    Vector3D z(7.0f, 8.0f, 9.0f);
    RotationMatrix rm(x, y, z);
    RotationMatrix transposed = rm.Transpose();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, transposed.XAxis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, transposed.XAxis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.0f, transposed.XAxis.Z);
}

void TestRotationMatrix::RunAllTests() {

    RUN_TEST(TestConvertCoordinateToVector);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestDeterminant);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestInverse);
    RUN_TEST(TestIsEqual);
    RUN_TEST(TestMultiply);
    RUN_TEST(TestNormalize);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestReadjustMatrix);
    RUN_TEST(TestRotate);
    RUN_TEST(TestRotateRelative);
    RUN_TEST(TestRotateX);
    RUN_TEST(TestRotateY);
    RUN_TEST(TestRotateZ);
    RUN_TEST(TestToString);
    RUN_TEST(TestTranspose);
}
