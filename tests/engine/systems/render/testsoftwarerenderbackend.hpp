// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsoftwarerenderbackend.hpp
 * @brief Unit tests for the SoftwareRenderBackend class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/gl/software_render_backend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSoftwareRenderBackend
 * @brief Contains static test methods for the SoftwareRenderBackend class.
 */
class TestSoftwareRenderBackend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestRender();
    static void TestReadPixels();
    static void TestGetName();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
