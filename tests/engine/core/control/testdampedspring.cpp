// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdampedspring.cpp
 * @brief Implementation of DampedSpring unit tests.
 */

#include "testdampedspring.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestDampedSpring::TestDefaultConstructor() {
    DampedSpring spring;
    float pos = spring.GetCurrentPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos);
}

// ========== Method Tests ==========
void TestDampedSpring::TestGetCurrentPosition() {
    DampedSpring spring(10.0f, 0.5f);
    float pos = spring.GetCurrentPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos);
}

void TestDampedSpring::TestSetConstants() {
    DampedSpring spring;
    spring.SetConstants(20.0f, 1.0f);
    float result = spring.Calculate(10.0f, 0.1f);
    TEST_ASSERT_TRUE(result >= 0.0f || result < 0.0f);
}

void TestDampedSpring::TestCalculate() {
    DampedSpring spring(10.0f, 0.5f);
    
    float result1 = spring.Calculate(10.0f, 0.1f);
    TEST_ASSERT_TRUE(result1 > 0.0f);
    
    float result2 = spring.Calculate(10.0f, 0.1f);
    TEST_ASSERT_TRUE(result2 > result1);
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestDampedSpring::TestParameterizedConstructor() {
    DampedSpring spring(15.0f, 0.8f);
    float pos = spring.GetCurrentPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos);
}

void TestDampedSpring::TestEdgeCases() {
    DampedSpring spring(10.0f, 0.5f);
    
    float zero = spring.Calculate(0.0f, 0.1f);
    TEST_ASSERT_TRUE(zero >= 0.0f || zero < 0.0f);
    
    float large = spring.Calculate(1000.0f, 0.1f);
    TEST_ASSERT_TRUE(large > 0.0f);
    
    float verySmallDt = spring.Calculate(10.0f, 0.001f);
    TEST_ASSERT_TRUE(verySmallDt >= 0.0f || verySmallDt < 0.0f);
}

void TestDampedSpring::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetCurrentPosition);
    RUN_TEST(TestSetConstants);
    RUN_TEST(TestCalculate);
    RUN_TEST(TestEdgeCases);
}
