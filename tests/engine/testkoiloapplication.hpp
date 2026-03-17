// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkoiloapplication.hpp
 * @brief Unit tests for the KoiloApplication class.
 *
 * @date 15/02/2026
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <koilo/app/koiloapplication.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKoiloApplication
 * @brief Contains static test methods for the KoiloApplication class.
 */
class TestKoiloApplication {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestLoadScript();
    static void TestUpdate();
    static void TestEnableHotReload();
    static void TestGetScene();
    static void TestGetEngine();
    static void TestGetMesh();
    static void TestHasError();
    static void TestGetError();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
