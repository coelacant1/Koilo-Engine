/**
 * @file testeasyeaseanimator.hpp
 * @brief Unit tests for the EasyEaseAnimator class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/animation/easyeaseanimator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEasyEaseAnimator
 * @brief Contains static test methods for the EasyEaseAnimator class.
 */
class TestEasyEaseAnimator {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetConstants();
    static void TestGetValue();
    static void TestGetTarget();
    static void TestAddParameter();
    static void TestAddParameterFrame();
    static void TestSetInterpolationMethod();
    static void TestReset();
    static void TestSetParameters();
    static void TestUpdate();
    static void TestGetCapacity();
    // ... add tests for remaining 3 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetParameterCount();
    static void TestIsActive();
    static void TestSetActive();
    static void RunAllTests();
};
