/**
 * @file testassetmanifest.hpp
 * @brief Unit tests for the AssetManifest class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/asset/asset_manifest.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAssetManifest
 * @brief Contains static test methods for the AssetManifest class.
 */
class TestAssetManifest {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestTotalBytes();
    static void TestClear();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
