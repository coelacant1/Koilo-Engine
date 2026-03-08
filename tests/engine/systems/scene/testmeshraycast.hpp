// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmeshraycast.hpp
 * @brief Unit tests for the MeshRaycast class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/meshraycast.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMeshRaycast
 * @brief Contains static test methods for the MeshRaycast class.
 */
class TestMeshRaycast {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
