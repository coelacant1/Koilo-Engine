// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmatrix4x4.hpp
 * @brief Unit tests for the Matrix4x4 class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/math/matrix4x4.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMatrix4x4
 * @brief Contains static test methods for the Matrix4x4 class.
 */
class TestMatrix4x4 {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetIdentity();
    static void TestTransformDirection();
    static void TestTranspose();
    static void TestDeterminant();
    static void TestInverse();
    static void TestGetTranslation();
    static void TestGetScale();
    static void TestGetRotation();
    static void TestIsEqual();
    static void TestIsIdentity();
    static void TestToString();
    static void TestTranslation();
    static void TestScale();
    static void TestRotation();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
