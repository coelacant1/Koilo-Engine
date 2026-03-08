// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testskeleton.hpp
 * @brief Unit tests for the Skeleton class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/skeleton.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSkeleton
 * @brief Contains static test methods for the Skeleton class.
 */
class TestSkeleton {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestGetBoneIndex();
    static void TestGetBoneCount();
    static void TestSkinVertices();

    static void TestResetPose();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestAddBone();
    static void TestComputeWorldMatrices();
    static void TestGetSkinMatrix();
    static void TestGetWorldMatrix();
    static void TestSetBindPose();
    static void RunAllTests();
};
