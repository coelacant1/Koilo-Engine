/**
 * @file testkeyframeinterpolation.hpp
 * @brief Unit tests for the KeyFrameInterpolation class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/keyframetrack.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKeyFrameInterpolation
 * @brief Contains static test methods for the KeyFrameInterpolation class.
 */
class TestKeyFrameInterpolation {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
