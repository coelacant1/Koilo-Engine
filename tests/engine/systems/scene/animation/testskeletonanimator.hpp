// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testskeletonanimator.hpp
 * @brief Unit tests for the SkeletonAnimator class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/skeleton_animator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSkeletonAnimator
 * @brief Contains static test methods for the SkeletonAnimator class.
 */
class TestSkeletonAnimator {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestApplyClip();

    static void TestSetSkeleton();
    static void TestGetSkeleton();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestApplyChannel();
    static void RunAllTests();
};
