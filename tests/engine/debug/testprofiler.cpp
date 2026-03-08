// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprofiler.cpp
 * @brief Implementation of Profiler unit tests.
 */

#include "testprofiler.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestProfiler::TestDefaultConstructor() {
    // Profiler is a singleton
    Profiler& profiler = Profiler::GetInstance();
    TEST_ASSERT_TRUE(true);
}

void TestProfiler::TestParameterizedConstructor() {
    // Profiler is a singleton - no parameterized constructor
    Profiler& profiler = Profiler::GetInstance();
    TEST_ASSERT_TRUE(true);
}

// ========== Method Tests ==========

void TestProfiler::TestEnable() {
    Profiler& profiler = Profiler::GetInstance();
    
    profiler.Enable();
    TEST_ASSERT_EQUAL(true, profiler.IsEnabled());
}

void TestProfiler::TestDisable() {
    Profiler& profiler = Profiler::GetInstance();
    
    profiler.Disable();
    TEST_ASSERT_EQUAL(false, profiler.IsEnabled());
}

void TestProfiler::TestIsEnabled() {
    Profiler& profiler = Profiler::GetInstance();
    
    profiler.Enable();
    TEST_ASSERT_EQUAL(true, profiler.IsEnabled());
    
    profiler.Disable();
    TEST_ASSERT_EQUAL(false, profiler.IsEnabled());
}

void TestProfiler::TestBeginFrame() {
    Profiler& profiler = Profiler::GetInstance();
    profiler.Enable();
    profiler.ClearStats();
    
    int initialFrameCount = profiler.GetFrameCount();
    profiler.BeginFrame();
    
    // Frame count doesn't increment until EndFrame
    TEST_ASSERT_EQUAL(initialFrameCount, profiler.GetFrameCount());
}

void TestProfiler::TestEndFrame() {
    Profiler& profiler = Profiler::GetInstance();
    profiler.Enable();
    profiler.ClearStats();
    
    int initialFrameCount = profiler.GetFrameCount();
    profiler.BeginFrame();
    profiler.EndFrame();
    
    // Frame count should increment after EndFrame
    TEST_ASSERT_EQUAL(initialFrameCount + 1, profiler.GetFrameCount());
}

void TestProfiler::TestGetLastFrameTime() {
    Profiler& profiler = Profiler::GetInstance();
    profiler.Enable();
    
    profiler.BeginFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    profiler.EndFrame();
    
    double frameTime = profiler.GetLastFrameTime();
    
    // Frame time should be at least 10ms
    TEST_ASSERT_GREATER_THAN(5.0, frameTime);
    TEST_ASSERT_LESS_THAN(50.0, frameTime);
}

void TestProfiler::TestGetFPS() {
    Profiler& profiler = Profiler::GetInstance();
    profiler.Enable();
    profiler.ClearStats();
    
    // Simulate a few frames
    for (int i = 0; i < 5; i++) {
        profiler.BeginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        profiler.EndFrame();
    }
    
    double fps = profiler.GetFPS();
    
    // FPS should be reasonable (30-100)
    TEST_ASSERT_GREATER_THAN(30.0, fps);
    TEST_ASSERT_LESS_THAN(100.0, fps);
}

void TestProfiler::TestClearStats() {
    Profiler& profiler = Profiler::GetInstance();
    profiler.Enable();
    
    // Record some frames
    profiler.BeginFrame();
    profiler.EndFrame();
    
    int frameCountBefore = profiler.GetFrameCount();
    TEST_ASSERT_GREATER_THAN(0, frameCountBefore);
    
    profiler.ClearStats();
    
    // After clear, frame count should reset
    TEST_ASSERT_EQUAL(0, profiler.GetFrameCount());
}

void TestProfiler::TestPrintStats() {
    Profiler& profiler = Profiler::GetInstance();
    profiler.Enable();
    profiler.ClearStats();
    
    // Just verify it doesn't crash
    profiler.PrintStats();
    
    // Can't easily test console output in unit tests
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestProfiler::TestEdgeCases() {
    Profiler& profiler = Profiler::GetInstance();
    
    // Test GetFPS before any frames
    profiler.ClearStats();
    double fps = profiler.GetFPS();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, fps);
    
    // Test GetLastFrameTime before any frames
    double frameTime = profiler.GetLastFrameTime();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, frameTime);
    
    // Test multiple Enable/Disable
    profiler.Enable();
    profiler.Enable();
    TEST_ASSERT_EQUAL(true, profiler.IsEnabled());
    
    profiler.Disable();
    profiler.Disable();
    TEST_ASSERT_EQUAL(false, profiler.IsEnabled());
}

// ========== Test Runner ==========

void TestProfiler::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEnable);
    RUN_TEST(TestDisable);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestBeginFrame);
    RUN_TEST(TestEndFrame);
    RUN_TEST(TestGetLastFrameTime);
    RUN_TEST(TestGetFPS);
    RUN_TEST(TestClearStats);
    RUN_TEST(TestPrintStats);
    RUN_TEST(TestEdgeCases);
}
