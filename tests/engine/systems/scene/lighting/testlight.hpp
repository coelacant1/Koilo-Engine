/**
 * @file testlight.hpp
 * @brief Unit tests for the Light class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/lighting/light.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestLight
 * @brief Contains static test methods for the Light class.
 */
class TestLight {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSet();
    static void TestSetIntensity();
    static void TestSetFalloff();
    static void TestMoveTo();
    static void TestTranslate();
    static void TestSetCurve();
    static void TestGetPosition();
    static void TestGetFalloff();
    static void TestGetCurveA();
    static void TestGetCurveB();
    // ... add tests for remaining 1 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetIntensity();
    static void RunAllTests();
};
