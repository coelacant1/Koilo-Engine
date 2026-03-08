// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkmeshloader.hpp
 * @brief Unit tests for the KoiloMeshLoader class.
 *
 * @date 15/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/assets/koilomesh_loader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKoiloMeshLoader
 * @brief Contains static test methods for the KoiloMeshLoader class.
 */
class TestKoiloMeshLoader {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestLoad();
    static void TestGetVertices();
    static void TestGetTriangles();
    static void TestGetVertexCount();
    static void TestGetTriangleCount();
    static void TestGetMorphCount();
    static void TestGetError();
    static void TestHasUVs();
    static void TestHasNormals();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
