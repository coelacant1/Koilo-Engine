// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscenenode.hpp
 * @brief Unit tests for the SceneNode class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/scenenode.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSceneNode
 * @brief Contains static test methods for the SceneNode class.
 */
class TestSceneNode {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetPosition();
    static void TestSetRotation();
    static void TestSetScale();
    static void TestGetPosition();
    static void TestGetChildCount();
    static void TestSetParent();
    static void TestGetParent();
    static void TestFindChild();
    static void TestSetMesh();
    static void TestGetMesh();
    static void TestSetScriptPath();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
