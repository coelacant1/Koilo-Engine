// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtimeline.hpp
 * @brief Unit tests for the Timeline class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/timeline.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTimeline
 * @brief Contains static test methods for the Timeline class.
 */
class TestTimeline {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestAddKeyframe();
    static void TestClearKeyframes();
    static void TestGetCurrentFrame();
    static void TestGetEndFrame();
    static void TestGetFPS();
    static void TestGetStartFrame();
    static void TestHandleInput();
    static void TestKeyframes();
    static void TestSetCurrentFrame();
    static void TestSetFPS();
    static void RunAllTests();
};
