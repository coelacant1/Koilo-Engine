// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testresourcemanager.hpp
 * @brief Unit tests for ResourceManager.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestResourceManager {
public:
    // Singleton Tests

    // Loading Tests

    // Caching Tests

    // Unloading Tests

    // Hot Reload Tests

    // Memory Tests

    static void TestSetMemoryLimit();
    
    // Loader Registration Tests

    // Resource Count Tests

    static void TestCheckHotReload();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestEnableHotReload();
    static void TestGarbageCollect();
    static void TestGetCachedResourceCount();
    static void TestGetTotalMemoryUsed();
    static void TestParameterizedConstructor();
    static void TestPrintStatistics();
    static void TestUnloadAllResources();
    static void RunAllTests();
};

