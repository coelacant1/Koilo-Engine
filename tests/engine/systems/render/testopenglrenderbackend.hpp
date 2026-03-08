// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testopenglrenderbackend.hpp
 * @brief Unit tests for the OpenGLRenderBackend class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/gl/opengl_render_backend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestOpenGLRenderBackend
 * @brief Contains static test methods for the OpenGLRenderBackend class.
 */
class TestOpenGLRenderBackend {
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
