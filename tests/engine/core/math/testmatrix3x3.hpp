// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/core/math/matrix3x3.hpp>
#include <utils/testhelpers.hpp>

class TestMatrix3x3 {
public:

    static void TestParameterizedConstructor();
    static void TestTranspose();
    static void TestDeterminant();
    static void TestInverse();

    static void RunAllTests() {
        RUN_TEST(TestDefaultIsIdentity);
        RUN_TEST(TestParameterizedConstructor);
        RUN_TEST(TestTranspose);
        RUN_TEST(TestDeterminant);
        RUN_TEST(TestInverse);
        RUN_TEST(TestMultiplyVector);
        RUN_TEST(TestMultiplyMatrix);
        RUN_TEST(TestFromQuaternionIdentity);
        RUN_TEST(TestFromQuaternionRotates);
        RUN_TEST(TestDiagonal);
    }
};
