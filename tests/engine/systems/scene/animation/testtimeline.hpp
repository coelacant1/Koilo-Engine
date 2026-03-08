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
    static void TestPlay();
    static void TestStop();
    static void TestPause();
    static void TestResume();
    static void TestUpdate();
    static void TestIsPlaying();
    static void TestSetLooping();
    static void TestIsLooping();
    static void TestSetDuration();
    static void TestGetDuration();
    static void TestSetCurrentTime();
    static void TestGetCurrentTime();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
