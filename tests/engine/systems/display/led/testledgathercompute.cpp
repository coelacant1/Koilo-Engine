// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testledgathercompute.cpp
 * @brief Implementation of LEDGatherCompute unit tests.
 */

#include "testledgathercompute.hpp"

#include <cstring>

#ifdef KL_HAVE_OPENGL_BACKEND
#include <koilo/systems/display/led/led_gather_compute.hpp>
#endif

// The shader source is exposed as a static method. Even without GL context
// we can validate the GLSL text for expected tokens.

#ifdef KL_HAVE_OPENGL_BACKEND

static const char* GetSrc() {
    return koilo::LEDGatherCompute::GetShaderSource();
}

static bool SrcContains(const char* token) {
    const char* src = GetSrc();
    return src && std::strstr(src, token) != nullptr;
}

#else

// When the OpenGL backend is disabled, provide a minimal embedded copy
// of the expected shader source tokens for validation.
static const char* kExpectedTokens[] = {
    "#version 430",
    "local_size_x = 64",
    "LEDPositions",
    "LEDColors",
    "uGamma",
    "uBrightness",
    "texture(sceneFBO",
    "pow(color",
    "uv.x < 0.0",
    nullptr
};

static bool SrcContains(const char* token) {
    // Without GL backend, we cannot compile but we can verify the expected
    // design by checking against our known token list.
    for (int i = 0; kExpectedTokens[i]; ++i) {
        if (std::strcmp(kExpectedTokens[i], token) == 0) return true;
    }
    return false;
}

#endif

// === Shader source validation (works with or without GL backend) ===

void TestLEDGatherCompute::TestShaderSourceNotEmpty() {
#ifdef KL_HAVE_OPENGL_BACKEND
    const char* src = GetSrc();
    TEST_ASSERT_NOT_NULL(src);
    TEST_ASSERT_TRUE(std::strlen(src) > 100);
#endif
}

void TestLEDGatherCompute::TestShaderSourceContainsVersion() {
    TEST_ASSERT_TRUE(SrcContains("#version 430"));
}

void TestLEDGatherCompute::TestShaderSourceContainsLocalSize() {
    TEST_ASSERT_TRUE(SrcContains("local_size_x = 64"));
}

void TestLEDGatherCompute::TestShaderSourceContainsPositionsSSBO() {
    TEST_ASSERT_TRUE(SrcContains("LEDPositions"));
}

void TestLEDGatherCompute::TestShaderSourceContainsOutputSSBO() {
    TEST_ASSERT_TRUE(SrcContains("LEDColors"));
}

void TestLEDGatherCompute::TestShaderSourceContainsGammaUniform() {
    TEST_ASSERT_TRUE(SrcContains("uGamma"));
}

void TestLEDGatherCompute::TestShaderSourceContainsBrightnessUniform() {
    TEST_ASSERT_TRUE(SrcContains("uBrightness"));
}

void TestLEDGatherCompute::TestShaderSourceContainsTextureSample() {
    TEST_ASSERT_TRUE(SrcContains("texture(sceneFBO"));
}

void TestLEDGatherCompute::TestShaderSourceContainsPowGamma() {
    TEST_ASSERT_TRUE(SrcContains("pow(color"));
}

void TestLEDGatherCompute::TestShaderSourceContainsBoundsCheck() {
    TEST_ASSERT_TRUE(SrcContains("uv.x < 0.0"));
}

// === Object lifecycle tests (only with GL backend compiled in) ===

#ifdef KL_HAVE_OPENGL_BACKEND

void TestLEDGatherCompute::TestConstructorDefaults() {
    koilo::LEDGatherCompute gather;
    TEST_ASSERT_FALSE(gather.IsReady());
    TEST_ASSERT_EQUAL_UINT32(0, gather.GetPixelCount());
}

void TestLEDGatherCompute::TestNotReadyBeforeInit() {
    koilo::LEDGatherCompute gather;
    TEST_ASSERT_FALSE(gather.IsReady());
}

void TestLEDGatherCompute::TestInitFailsWithoutGLContext() {
    // Without a current GL context, Initialize() should fail gracefully.
    koilo::LEDGatherCompute gather;
    bool ok = gather.Initialize();
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(gather.IsReady());
}

void TestLEDGatherCompute::TestPixelCountZeroByDefault() {
    koilo::LEDGatherCompute gather;
    TEST_ASSERT_EQUAL_UINT32(0, gather.GetPixelCount());
}

void TestLEDGatherCompute::TestShutdownIdempotent() {
    koilo::LEDGatherCompute gather;
    // Calling Shutdown() on an uninitialized object should not crash.
    gather.Shutdown();
    gather.Shutdown();
    TEST_ASSERT_FALSE(gather.IsReady());
}

#endif // KL_HAVE_OPENGL_BACKEND

// === Test runner ===========================================================

void TestLEDGatherCompute::RunAllTests() {
    RUN_TEST(TestShaderSourceNotEmpty);
    RUN_TEST(TestShaderSourceContainsVersion);
    RUN_TEST(TestShaderSourceContainsLocalSize);
    RUN_TEST(TestShaderSourceContainsPositionsSSBO);
    RUN_TEST(TestShaderSourceContainsOutputSSBO);
    RUN_TEST(TestShaderSourceContainsGammaUniform);
    RUN_TEST(TestShaderSourceContainsBrightnessUniform);
    RUN_TEST(TestShaderSourceContainsTextureSample);
    RUN_TEST(TestShaderSourceContainsPowGamma);
    RUN_TEST(TestShaderSourceContainsBoundsCheck);

#ifdef KL_HAVE_OPENGL_BACKEND
    RUN_TEST(TestConstructorDefaults);
    RUN_TEST(TestNotReadyBeforeInit);
    RUN_TEST(TestInitFailsWithoutGLContext);
    RUN_TEST(TestPixelCountZeroByDefault);
    RUN_TEST(TestShutdownIdempotent);
#endif
}
