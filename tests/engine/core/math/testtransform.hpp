/**
 * @file testtransform.hpp
 * @brief Unit tests for the Transform class.
 *
 * Tests for 3D transformations including position, rotation, scale,
 * and their various offsets.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/transform.hpp>
#include <utils/testhelpers.hpp>

class TestTransform {
public:
    // Constructor tests
    static void TestDefaultConstructor();

    // Position tests

    static void TestTranslate();

    // Rotation tests

    static void TestRotate();

    // Scale tests

    // Offset tests

    // Integration tests

    static void TestToString();

    static void TestEdgeCases();
    static void TestGetBaseRotation();
    static void TestGetPosition();
    static void TestGetRotation();
    static void TestGetRotationOffset();
    static void TestGetScale();
    static void TestGetScaleOffset();
    static void TestGetScaleRotationOffset();
    static void TestParameterizedConstructor();
    static void TestScale();
    static void TestSetBaseRotation();
    static void TestSetPosition();
    static void TestSetRotation();
    static void TestSetRotationOffset();
    static void TestSetScale();
    static void TestSetScaleOffset();
    static void TestSetScaleRotationOffset();
    static void RunAllTests();
};
