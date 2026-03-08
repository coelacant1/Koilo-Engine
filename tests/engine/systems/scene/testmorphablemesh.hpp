// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmorphablemesh.hpp
 * @brief Unit tests for the MorphableMesh class.
 *
 * @date 16/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/morphablemesh.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMorphableMesh
 * @brief Contains static test methods for the MorphableMesh class.
 */
class TestMorphableMesh {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestLoad();
    static void TestIsLoaded();
    static void TestSetMorphWeight();
    static void TestGetMorphWeight();
    static void TestGetMorphCount();
    static void TestGetVertexCount();
    static void TestUpdate();
    static void TestResetMorphs();
    static void TestGetMesh();
    static void TestGetError();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
