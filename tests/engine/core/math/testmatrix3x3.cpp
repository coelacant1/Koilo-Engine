// SPDX-License-Identifier: GPL-3.0-or-later
#include "testmatrix3x3.hpp"
#include <cmath>

using namespace koilo;

void TestMatrix3x3::TestParameterizedConstructor() {
    Matrix3x3 m(1,2,3, 4,5,6, 7,8,9);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, m.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 5.0, m.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 9.0, m.M[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 6.0, m.M[1][2]);
}

void TestMatrix3x3::TestTranspose() {
    Matrix3x3 m(1,2,3, 4,5,6, 7,8,9);
    Matrix3x3 t = m.Transpose();
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, t.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 4.0, t.M[0][1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 7.0, t.M[0][2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 2.0, t.M[1][0]);
}

void TestMatrix3x3::TestDeterminant() {
    Matrix3x3 id;
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, id.Determinant());
    Matrix3x3 m(2,0,0, 0,3,0, 0,0,4);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 24.0, m.Determinant());
    Matrix3x3 sing(1,2,3, 2,4,6, 7,8,9); // row1 = 2*row0
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.0, sing.Determinant());
}

void TestMatrix3x3::TestInverse() {
    Matrix3x3 m(4,7,2, 3,6,1, 2,5,1);
    Matrix3x3 inv = m.Inverse();
    Matrix3x3 prod = m * inv;
    Matrix3x3 id;
    TEST_ASSERT_TRUE(prod.IsEqual(id, 1e-4f));
}

