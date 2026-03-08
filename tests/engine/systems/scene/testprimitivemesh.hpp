// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprimitivemesh.hpp
 * @brief Unit tests for the PrimitiveMesh class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/primitivemesh.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPrimitiveMesh
 * @brief Contains static test methods for the PrimitiveMesh class.
 */
class TestPrimitiveMesh {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestCreateQuad();
    static void TestCreateTexturedQuad();
    static void TestGetMesh();
    static void TestHasUV();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
