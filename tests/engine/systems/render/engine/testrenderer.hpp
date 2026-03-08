// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrenderer.hpp
 * @brief Unit tests for RenderingEngine class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>

namespace koilo {

class TestRenderingEngine {
public:
    static void TestRasterizeNullScene();
    static void TestRasterizeNullCameraManager();
    static void TestRasterizeEmptyScene();
    static void TestRasterizeWithCamera();
    static void TestRayTraceNullScene();
    static void TestRayTraceNullCameraManager();
    static void TestRayTraceWithCamera();
    static void RunAllTests();
};

} // namespace koilo
