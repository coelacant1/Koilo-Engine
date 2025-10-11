/**
 * @file testmesh.hpp
 * @brief Unit tests for the Mesh class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/mesh.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMesh
 * @brief Contains static test methods for the Mesh class.
 */
class TestMesh {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestEnable();
    static void TestDisable();
    static void TestIsEnabled();
    static void TestHasUV();
    static void TestGetUVVertices();
    static void TestGetUVIndexGroup();
    static void TestGetCenterOffset();
    static void TestGetMinMaxDimensions();
    static void TestGetSize();
    static void TestGetTransform();
    // ... add tests for remaining 6 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetMaterial();
    static void TestGetTriangleGroup();
    static void TestResetVertices();
    static void TestSetMaterial();
    static void TestSetTransform();
    static void TestUpdateTransform();
    static void RunAllTests();
};
