// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testanimationclip.hpp
 * @brief Unit tests for the AnimationClip class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/animationclip.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAnimationClip
 * @brief Contains static test methods for the AnimationClip class.
 */
class TestAnimationClip {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetName();
    static void TestSetName();
    static void TestGetDuration();
    static void TestSetDuration();
    static void TestGetLooping();
    static void TestSetLooping();
    static void TestAddChannel();
    static void TestGetChannel();
    static void TestGetChannelCount();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
