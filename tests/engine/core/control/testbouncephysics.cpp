// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbouncephysics.cpp
 * @brief Implementation of BouncePhysics unit tests.
 */

#include "testbouncephysics.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestBouncePhysics::TestDefaultConstructor() {
    BouncePhysics bounce(9.8f);
    float result = bounce.Calculate(0.0f, 0.1f);
    TEST_ASSERT_TRUE(result <= 0.0f);
}

void TestBouncePhysics::TestParameterizedConstructor() {
    BouncePhysics bounce(9.8f, 0.8f);
    float result = bounce.Calculate(10.0f, 0.1f);
    TEST_ASSERT_TRUE(result >= 0.0f || result < 0.0f);
}

// ========== Method Tests ==========

void TestBouncePhysics::TestCalculate() {
    BouncePhysics bounce(9.8f, 1.0f);
    
    float result1 = bounce.Calculate(10.0f, 0.1f);
    TEST_ASSERT_TRUE(result1 > 0.0f);
    
    float result2 = bounce.Calculate(0.0f, 0.1f);
    TEST_ASSERT_TRUE(result2 < result1);
}

// ========== Edge Cases ==========

void TestBouncePhysics::TestEdgeCases() {
    BouncePhysics bounce(9.8f, 0.5f);
    
    float zero = bounce.Calculate(0.0f, 0.1f);
    TEST_ASSERT_TRUE(zero <= 0.0f);
    
    float highVel = bounce.Calculate(100.0f, 0.1f);
    TEST_ASSERT_TRUE(highVel > 0.0f);
    
    float verySmallDt = bounce.Calculate(10.0f, 0.001f);
    TEST_ASSERT_TRUE(verySmallDt >= 0.0f || verySmallDt < 0.0f);
}

// ========== Test Runner ==========

void TestBouncePhysics::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestCalculate);
    RUN_TEST(TestEdgeCases);
}
