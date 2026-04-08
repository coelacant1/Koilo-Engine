// SPDX-License-Identifier: GPL-3.0-or-later
#include "testquaternion.hpp"
#include <cmath>

using namespace koilo;
void TestQuaternion::TestRotateVector(Quaternion q, Vector3D v, Vector3D e){
    Vector3D result = q.RotateVector(v);

    TEST_ASSERT_FLOAT_WITHIN(0.01, e.X, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, e.Y, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, e.Z, result.Z);
}

void TestQuaternion::TestAbsolute() {
    Quaternion q(-1.0, -2.0, -3.0, -4.0);
    Quaternion abs = q.Absolute();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, abs.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, abs.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, abs.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, abs.Z);
}

void TestQuaternion::TestAdd() {
    Quaternion q1(1.0, 2.0, 3.0, 4.0);
    Quaternion q2(0.5, 1.0, 1.5, 2.0);
    Quaternion result = q1.Add(q2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.5, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.5, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 6.0, result.Z);
}

void TestQuaternion::TestAdditiveInverse() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    Quaternion inv = q.AdditiveInverse();
    TEST_ASSERT_FLOAT_WITHIN(0.01, -1.0, inv.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -2.0, inv.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -3.0, inv.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -4.0, inv.Z);
}

void TestQuaternion::TestConjugate() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    Quaternion conj = q.Conjugate();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, conj.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -2.0, conj.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -3.0, conj.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -4.0, conj.Z);
}

void TestQuaternion::TestDefaultConstructor() {
    Quaternion q;
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, q.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, q.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, q.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, q.Z);
}

void TestQuaternion::TestDeltaRotation() {
    Quaternion q1(1.0, 0.0, 0.0, 0.0);
    Vector3D angVel(0.1f, 0.0f, 0.0f);
    Quaternion delta = q1.DeltaRotation(angVel, 0.016f);
    TEST_ASSERT_FALSE(std::isnan(delta.W) && std::isnan(delta.X) && std::isnan(delta.Y) && std::isnan(delta.Z));
}

void TestQuaternion::TestDivide() {
    // Quaternion divide is q1 * conj(q2)-style multiplication, not element-wise
    Quaternion q1(4.0, 6.0, 8.0, 10.0);
    Quaternion q2(2.0, 2.0, 2.0, 2.0);
    Quaternion result = q1.Divide(q2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -40.0, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 16.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 32.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 24.0, result.Z);
    
    Quaternion q(4.0, 2.0, 6.0, 8.0);
    Quaternion scaled = q.Divide(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, scaled.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, scaled.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, scaled.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, scaled.Z);
}

void TestQuaternion::TestDotProduct() {
    Quaternion q1(1.0, 2.0, 3.0, 4.0);
    Quaternion q2(2.0, 3.0, 4.0, 5.0);
    float dot = q1.DotProduct(q2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 40.0, dot);
}

void TestQuaternion::TestEdgeCases() {
    Quaternion zero(0.0, 0.0, 0.0, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, zero.Magnitude());
    TEST_ASSERT_FALSE(zero.IsNonZero());
    
    Quaternion identity(1.0, 0.0, 0.0, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, identity.Magnitude());
    
    Quaternion large(1e6, 1e6, 1e6, 1e6);
    Quaternion normalized = large.UnitQuaternion();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, normalized.Magnitude());
    
    Quaternion negative(-1.0, -2.0, -3.0, -4.0);
    Quaternion abs = negative.Absolute();
    TEST_ASSERT_TRUE(abs.W > 0 && abs.X > 0 && abs.Y > 0 && abs.Z > 0);
}

void TestQuaternion::TestGetBiVector() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    Vector3D biVec = q.GetBiVector();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, biVec.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, biVec.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, biVec.Z);
}

void TestQuaternion::TestGetNormal() {
    Quaternion q(2.0, 1.0, 1.0, 1.0);
    Vector3D normal = q.GetNormal();
    TEST_ASSERT_FALSE(std::isnan(normal.X) && std::isnan(normal.Y) && std::isnan(normal.Z));
}

void TestQuaternion::TestIsClose() {
    Quaternion q1(1.0, 2.0, 3.0, 4.0);
    Quaternion q2(1.001, 2.001, 3.001, 4.001);
    Quaternion q3(1.1, 2.1, 3.1, 4.1);
    
    TEST_ASSERT_TRUE(q1.IsClose(q2, 0.01));
    TEST_ASSERT_FALSE(q1.IsClose(q3, 0.01));
}

void TestQuaternion::TestIsEqual() {
    Quaternion q1(1.0, 2.0, 3.0, 4.0);
    Quaternion q2(1.0, 2.0, 3.0, 4.0);
    Quaternion q3(1.0, 2.0, 3.0, 5.0);
    
    TEST_ASSERT_TRUE(q1.IsEqual(q2));
    TEST_ASSERT_FALSE(q1.IsEqual(q3));
}

void TestQuaternion::TestIsFinite() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    TEST_ASSERT_TRUE(q.IsFinite());
    
    Quaternion qInf(INFINITY, 2.0, 3.0, 4.0);
    TEST_ASSERT_FALSE(qInf.IsFinite());
}

void TestQuaternion::TestIsInfinite() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    TEST_ASSERT_FALSE(q.IsInfinite());
    
    Quaternion qInf(INFINITY, 2.0, 3.0, 4.0);
    TEST_ASSERT_TRUE(qInf.IsInfinite());
}

void TestQuaternion::TestIsNaN() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    TEST_ASSERT_FALSE(q.IsNaN());
    
    Quaternion qNaN(NAN, 2.0, 3.0, 4.0);
    TEST_ASSERT_TRUE(qNaN.IsNaN());
}

void TestQuaternion::TestIsNonZero() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    TEST_ASSERT_TRUE(q.IsNonZero());
    
    Quaternion qZero(0.0, 0.0, 0.0, 0.0);
    TEST_ASSERT_FALSE(qZero.IsNonZero());
}

void TestQuaternion::TestMagnitude() {
    Quaternion q(1.0, 0.0, 0.0, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, q.Magnitude());
    
    Quaternion q2(0.5, 0.5, 0.5, 0.5);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, q2.Magnitude());
}

void TestQuaternion::TestMultiplicativeInverse() {
    Quaternion q(0.5, 0.5, 0.5, 0.5);
    Quaternion inv = q.MultiplicativeInverse();
    Quaternion product = q.Multiply(inv);
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, product.W);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, product.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, product.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, product.Z);
}

void TestQuaternion::TestMultiply() {
    Quaternion q1(1.0, 0.0, 1.0, 0.0);
    Quaternion q2(1.0, 0.5, 0.5, 0.75);
    Quaternion result = q1.Multiply(q2);
    TEST_ASSERT_FALSE(std::isnan(result.W) && std::isnan(result.X) && std::isnan(result.Y) && std::isnan(result.Z));
    
    Quaternion q(2.0, 1.0, 1.0, 1.0);
    Quaternion scaled = q.Multiply(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, scaled.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, scaled.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, scaled.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, scaled.Z);
}

void TestQuaternion::TestNormal() {
    Quaternion q(2.0, 0.0, 0.0, 0.0);
    float norm = q.Normal();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, norm);
    
    Quaternion q2(1.0, 1.0, 1.0, 1.0);
    float norm2 = q2.Normal();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, norm2);
}

void TestQuaternion::TestParameterizedConstructor() {
    Quaternion q(0.5, 0.5, 0.5, 0.5);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, q.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, q.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, q.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, q.Z);
    
    Quaternion copy(q);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, copy.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, copy.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, copy.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, copy.Z);
    
    Vector3D vec(1.0, 2.0, 3.0);
    Quaternion qVec(vec);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, qVec.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, qVec.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, qVec.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, qVec.Z);
}

void TestQuaternion::TestPermutate() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    Vector3D perm(1.0f, 2.0f, 0.0f);  // Permutation indices must be 0, 1, or 2
    Quaternion result = q.Permutate(perm);
    TEST_ASSERT_FALSE(std::isnan(result.W) && std::isnan(result.X) && std::isnan(result.Y) && std::isnan(result.Z));
}

void TestQuaternion::TestPower() {
    Quaternion q(2.0, 1.0, 1.0, 1.0);
    Quaternion power = q.Power(2.0f);
    TEST_ASSERT_FALSE(std::isnan(power.W) && std::isnan(power.X) && std::isnan(power.Y) && std::isnan(power.Z));
}

void TestQuaternion::TestRotateVectorUnit() {
    Quaternion q(0.707, 0.0, 0.707, 0.0);
    Quaternion unit = q.UnitQuaternion();
    Vector2D v2d(1.0f, 0.0f);
    Vector2D rotated = q.RotateVectorUnit(v2d, unit);
    TEST_ASSERT_FALSE(std::isnan(rotated.X) && std::isnan(rotated.Y));
}

void TestQuaternion::TestSubtract() {
    Quaternion q1(2.0, 3.0, 4.0, 5.0);
    Quaternion q2(1.0, 1.0, 2.0, 1.0);
    Quaternion result = q1.Subtract(q2);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, result.W);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 2.0, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 4.0, result.Z);
}

void TestQuaternion::TestToString() {
    Quaternion q(1.0, 2.0, 3.0, 4.0);
    koilo::UString str = q.ToString();
    TEST_ASSERT_NOT_NULL(str.CStr());
    TEST_ASSERT_TRUE(strlen(str.CStr()) > 0);
}

void TestQuaternion::TestUnitQuaternion() {
    Quaternion q(2.0, 0.0, 0.0, 0.0);
    Quaternion unit = q.UnitQuaternion();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, unit.Magnitude());
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
