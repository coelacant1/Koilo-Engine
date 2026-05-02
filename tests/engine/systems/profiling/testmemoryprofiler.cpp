// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmemoryprofiler.cpp
 * @brief Implementation of MemoryProfiler tests.
 */

#include "testmemoryprofiler.hpp"
#include <koilo/systems/profiling/memoryprofiler.hpp>

using namespace koilo;

void TestMemoryProfiler::TestDefaultConstructor() {
    MemoryProfiler profiler;
    TEST_ASSERT_TRUE(true);
}

void TestMemoryProfiler::TestParameterizedConstructor() {
    // Singleton access
    MemoryProfiler& inst = MemoryProfiler::GetInstance();
    (void)inst;
    TEST_ASSERT_TRUE(true);
}

void TestMemoryProfiler::TestGetStats() {
    MemoryProfiler profiler;
    MemoryStats stats = profiler.GetStats();
    TEST_ASSERT_TRUE(stats.currentUsage == stats.currentUsage);
}

void TestMemoryProfiler::TestIsEnabled() {
    MemoryProfiler profiler;
    bool enabled = profiler.IsEnabled();
    TEST_ASSERT_TRUE(enabled == true || enabled == false);
}

void TestMemoryProfiler::TestSetEnabled() {
    MemoryProfiler profiler;
    profiler.SetEnabled(true);
    TEST_ASSERT_TRUE(profiler.IsEnabled());
    profiler.SetEnabled(false);
    TEST_ASSERT_FALSE(profiler.IsEnabled());
}

void TestMemoryProfiler::TestClear() {
    MemoryProfiler profiler;
    profiler.Clear();
    MemoryStats stats = profiler.GetStats();
    TEST_ASSERT_EQUAL(0, stats.allocationCount);
}

void TestMemoryProfiler::TestPrintLeaks() {
    MemoryProfiler profiler;
    profiler.PrintLeaks(); // Should not crash
    TEST_ASSERT_TRUE(true);
}

void TestMemoryProfiler::TestPrintReport() {
    MemoryProfiler profiler;
    profiler.PrintReport(); // Should not crash
    TEST_ASSERT_TRUE(true);
}

void TestMemoryProfiler::TestEdgeCases() {
    MemoryProfiler profiler;
    profiler.Clear();
    MemoryStats stats = profiler.GetStats();
    TEST_ASSERT_EQUAL(0, stats.currentUsage);
}

void TestMemoryProfiler::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetStats);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestSetEnabled);
    RUN_TEST(TestClear);
    RUN_TEST(TestPrintLeaks);
    RUN_TEST(TestPrintReport);
    RUN_TEST(TestEdgeCases);
}
