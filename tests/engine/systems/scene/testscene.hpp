/**
 * @file testscene.hpp
 * @brief Unit tests for the Scene class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/scene.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestScene
 * @brief Contains static test methods for the Scene class.
 */
class TestScene {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddMesh();
    static void TestRemoveMesh();
    static void TestGetMeshes();
    static void TestGetMeshCount();
    static void TestGetTotalTriangleCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
