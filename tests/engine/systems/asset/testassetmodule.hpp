/**
 * @file testassetmodule.hpp
 * @brief Unit tests for the AssetModule class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/asset/asset_module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAssetModule
 * @brief Contains static test methods for the AssetModule class.
 */
class TestAssetModule {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestScriptLoad();
    static void TestScriptUnload();
    static void TestScriptIsValid();
    static void TestScriptGetType();
    static void TestScriptGetState();
    static void TestScriptGetPath();
    static void TestGetAssetCount();
    static void TestGetTotalMemory();
    static void TestReloadChanged();
    static void TestGarbageCollect();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
