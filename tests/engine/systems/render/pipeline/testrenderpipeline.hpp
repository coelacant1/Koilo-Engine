/**
 * @file testrenderpipeline.hpp
 * @brief Unit tests for the RenderPipeline class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRenderPipeline
 * @brief Contains static test methods for the RenderPipeline class.
 */
class TestRenderPipeline {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestRender();
    static void TestReadPixels();
    static void TestGetName();
    static void TestBlitToScreen();
    static void TestCompositeCanvasOverlays();
    static void TestPrepareFrame();
    static void TestFinishFrame();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
