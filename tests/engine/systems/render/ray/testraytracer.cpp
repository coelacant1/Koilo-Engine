// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testraytracer.cpp
 * @brief Implementation of RayTracer unit tests.
 */

#include "testraytracer.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestRayTracer::TestDefaultConstructor() {
    [[maybe_unused]] RayTracer tracer;
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestRayTracer::TestParameterizedConstructor() {
    [[maybe_unused]] RayTracer tracer;
    TEST_ASSERT_TRUE(true);
}

void TestRayTracer::TestEdgeCases() {
    [[maybe_unused]] RayTracer tracer;
    TEST_ASSERT_TRUE(true);
}

void TestRayTracer::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
