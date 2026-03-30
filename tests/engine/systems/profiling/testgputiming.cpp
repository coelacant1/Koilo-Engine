// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testgputiming.cpp
 * @brief Unit tests for GPUTimingManager.
 */

#include "testgputiming.hpp"
#include <koilo/systems/profiling/gpu_timing.hpp>

using namespace koilo;

void TestGPUTiming::TestDefaultState() {
    GPUTimingManager timing;
    TEST_ASSERT_FALSE(timing.IsEnabled());
    TEST_ASSERT_TRUE(timing.GetPassTimings().empty());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, (float)timing.GetTotalGPUTimeMs());
}

void TestGPUTiming::TestEnableDisable() {
    GPUTimingManager timing;
    timing.SetEnabled(true);
    TEST_ASSERT_TRUE(timing.IsEnabled());
    timing.SetEnabled(false);
    TEST_ASSERT_FALSE(timing.IsEnabled());
}

void TestGPUTiming::TestBeginEndFrameWithNullDevice() {
    GPUTimingManager timing;
    timing.SetEnabled(true);
    // Passing nullptr should not crash - enabled_ stays true but no queries run
    timing.BeginFrame(nullptr);
    timing.EndFrame();
    TEST_ASSERT_TRUE(timing.GetPassTimings().empty());
}

void TestGPUTiming::TestPassTimingWithNullDevice() {
    GPUTimingManager timing;
    timing.SetEnabled(true);
    timing.BeginFrame(nullptr);
    // BeginPass / EndPass with null device should be safe no-ops
    timing.BeginPass("scene");
    timing.EndPass("scene");
    timing.EndFrame();
    TEST_ASSERT_TRUE(timing.GetPassTimings().empty());
}

void TestGPUTiming::TestTotalTimeDefault() {
    GPUTimingManager timing;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, (float)timing.GetTotalGPUTimeMs());
}

void TestGPUTiming::TestMultiplePasses() {
    GPUTimingManager timing;
    timing.SetEnabled(true);
    // Without a device, passes should not accumulate timestamps
    timing.BeginFrame(nullptr);
    timing.BeginPass("scene");
    timing.EndPass("scene");
    timing.BeginPass("overlay");
    timing.EndPass("overlay");
    timing.EndFrame();
    // No resolved data without a real device
    TEST_ASSERT_TRUE(timing.GetPassTimings().empty());
}

void TestGPUTiming::RunAllTests() {
    RUN_TEST(TestGPUTiming::TestDefaultState);
    RUN_TEST(TestGPUTiming::TestEnableDisable);
    RUN_TEST(TestGPUTiming::TestBeginEndFrameWithNullDevice);
    RUN_TEST(TestGPUTiming::TestPassTimingWithNullDevice);
    RUN_TEST(TestGPUTiming::TestTotalTimeDefault);
    RUN_TEST(TestGPUTiming::TestMultiplePasses);
}
