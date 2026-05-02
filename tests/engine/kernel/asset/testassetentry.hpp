/**
 * @file testassetentry.hpp
 * @brief Unit tests for the AssetEntry class.
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
 * @class TestAssetEntry
 * @brief Contains static test methods for the AssetEntry class.
 */
class TestAssetEntry {
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
