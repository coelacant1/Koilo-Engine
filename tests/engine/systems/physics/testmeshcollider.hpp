/**
 * @file testmeshcollider.hpp
 * @brief Unit tests for the MeshCollider class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/meshcollider.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMeshCollider
 * @brief Contains static test methods for the MeshCollider class.
 */
class TestMeshCollider {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPosition();
    static void TestSetPosition();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
