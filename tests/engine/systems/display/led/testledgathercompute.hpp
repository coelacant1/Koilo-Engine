// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testledgathercompute.hpp
 * @brief Unit tests for LEDGatherCompute GPU pixel gather.
 */

#pragma once

#include <unity.h>

/**
 * @class TestLEDGatherCompute
 * @brief Tests for the GPU gather compute shader utility.
 *
 * Since unit tests run without a GL context, these tests validate
 * the shader source, construction/destruction behavior, and the
 * fallback logic. Full GPU path testing requires an integration test.
 */
class TestLEDGatherCompute {
public:
    static void RunAllTests();

    static void TestShaderSourceNotEmpty();
    static void TestShaderSourceContainsVersion();
    static void TestShaderSourceContainsLocalSize();
    static void TestShaderSourceContainsPositionsSSBO();
    static void TestShaderSourceContainsOutputSSBO();
    static void TestShaderSourceContainsGammaUniform();
    static void TestShaderSourceContainsBrightnessUniform();
    static void TestShaderSourceContainsTextureSample();
    static void TestShaderSourceContainsPowGamma();
    static void TestShaderSourceContainsBoundsCheck();

#ifdef KL_HAVE_OPENGL_BACKEND
    static void TestConstructorDefaults();
    static void TestNotReadyBeforeInit();
    static void TestInitFailsWithoutGLContext();
    static void TestPixelCountZeroByDefault();
    static void TestShutdownIdempotent();
#endif
};
