/**
 * @file testmeshcache.hpp
 * @brief Unit tests for the MeshCache class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/pipeline/mesh_cache.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMeshCache
 * @brief Contains static test methods for the MeshCache class.
 */
class TestMeshCache {
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
