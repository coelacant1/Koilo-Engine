/**
 * @file testtexturecache.hpp
 * @brief Unit tests for the TextureCache class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/pipeline/texture_cache.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTextureCache
 * @brief Contains static test methods for the TextureCache class.
 */
class TestTextureCache {
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
