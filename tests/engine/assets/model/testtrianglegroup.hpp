/**
 * @file testtrianglegroup.hpp
 * @brief Unit tests for the TriangleGroup class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/assets/model/trianglegroup.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTriangleGroup
 * @brief Contains static test methods for the TriangleGroup class.
 */
class TestTriangleGroup {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetIndexGroup();
    static void TestGetTriangleCount();
    static void TestGetVertices();
    static void TestGetVertexCount();
    static void TestGetTriangles();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
