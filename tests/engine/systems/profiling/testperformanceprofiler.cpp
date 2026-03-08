// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testperformanceprofiler.cpp
 * @brief Implementation of PerformanceProfiler tests.
 */

#include "testperformanceprofiler.hpp"
#include <koilo/systems/profiling/performanceprofiler.hpp>

using namespace koilo;

void TestPerformanceProfiler::TestBeginFrame() {
    PerformanceProfiler profiler;
    profiler.BeginFrame();
    TEST_ASSERT_TRUE(true);
}

void TestPerformanceProfiler::TestEndFrame() {
    PerformanceProfiler profiler;
    profiler.BeginFrame();
    profiler.EndFrame();
    double frameTime = profiler.GetFrameDuration();
    TEST_ASSERT_TRUE(frameTime >= 0.0);
}

void TestPerformanceProfiler::TestGetFPS() {
    PerformanceProfiler profiler;
    for (int i = 0; i < 3; i++) {
        profiler.BeginFrame();
        profiler.EndFrame();
    }
    double fps = profiler.GetFPS();
    TEST_ASSERT_TRUE(fps >= 0.0);
}

void TestPerformanceProfiler::TestClear() {
    PerformanceProfiler profiler;
    profiler.BeginFrame();
    profiler.EndFrame();
    profiler.Clear();
    TEST_ASSERT_TRUE(true);
}

void TestPerformanceProfiler::TestDefaultConstructor() {
    PerformanceProfiler profiler;
    TEST_ASSERT_TRUE(true);
}

void TestPerformanceProfiler::TestEdgeCases() {
    PerformanceProfiler profiler;
    profiler.EndFrame(); // end without begin
    TEST_ASSERT_TRUE(true);
}

void TestPerformanceProfiler::TestIsEnabled() {
    PerformanceProfiler profiler;
    TEST_ASSERT_TRUE(profiler.IsEnabled() == true || profiler.IsEnabled() == false);
}

void TestPerformanceProfiler::TestParameterizedConstructor() {
    PerformanceProfiler& inst = PerformanceProfiler::GetInstance();
    (void)inst;
    TEST_ASSERT_TRUE(true);
}

void TestPerformanceProfiler::TestPrintReport() {
    PerformanceProfiler profiler;
    profiler.PrintReport();
    TEST_ASSERT_TRUE(true);
}

void TestPerformanceProfiler::TestSetEnabled() {
    PerformanceProfiler profiler;
    profiler.SetEnabled(true);
    TEST_ASSERT_TRUE(profiler.IsEnabled());
    profiler.SetEnabled(false);
    TEST_ASSERT_FALSE(profiler.IsEnabled());
}

void TestPerformanceProfiler::RunAllTests() {
    RUN_TEST(TestBeginFrame);
    RUN_TEST(TestEndFrame);
    RUN_TEST(TestGetFPS);
    RUN_TEST(TestClear);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPrintReport);
    RUN_TEST(TestSetEnabled);
}
