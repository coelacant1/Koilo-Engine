// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmoduleloader.hpp
 * @brief Unit tests for the ModuleLoader class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/module_loader.hpp>
#include <koilo/kernel/module_api.hpp>
#include <koilo/kernel/module_abi_adapters.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestModuleLoader
 * @brief Contains static test methods for the ModuleLoader class.
 */
class TestModuleLoader {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestUpdateAll();
    static void TestRenderAll();
    static void TestShutdownAll();
    static void TestHasModule();
    static void TestListModules();
    static void TestGetModule();
    static void TestUnloadModule();
    static void TestLoadFromLibrary();
    static void TestScanAndLoad();
    static void TestTryLoad();
    static void TestSetModuleSearchPath();
    static void TestGetModuleSearchPath();
    static void TestSetLoadMode();
    static void TestGetLoadMode();
    static void TestReloadModule();
    static void TestCheckAndReload();

    // Edge case & integration tests
    static void TestEdgeCases();

    // ABI v3 tests

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
