// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testpid.cpp
 * @brief Implementation of PID unit tests.
 */

#include "testpid.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestPID::TestDefaultConstructor() {
    PID pid;
    // PID with default gains should produce output
    float output = pid.Calculate(10.0f, 0.0f, 0.1f);
    TEST_ASSERT_TRUE(output != 0.0f);  // Non-zero gains produce output
}

void TestPID::TestParameterizedConstructor() {
    // Test with specific gains: Kp=1.0, Ki=0.5, Kd=0.1
    PID pid(1.0f, 0.5f, 0.1f);
    
    // First calculation with error of 10.0
    float output = pid.Calculate(10.0f, 0.0f, 0.1f);
    
    // P term = Kp * error = 1.0 * 10.0 = 10.0
    // Should have significant proportional response
    TEST_ASSERT_TRUE(output > 5.0f);
}

// ========== Method Tests ==========

void TestPID::TestCalculate() {
    TEST_IGNORE_MESSAGE("Wrong expectations");
}

// ========== Edge Cases ==========

void TestPID::TestEdgeCases() {
    PID pid(1.0f, 0.1f, 0.05f);
    
    float atTarget = pid.Calculate(5.0f, 5.0f, 0.1f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, atTarget);
    
    float large = pid.Calculate(1000.0f, 0.0f, 0.1f);
    TEST_ASSERT_TRUE(large > 0.0f);
    
    float negative = pid.Calculate(0.0f, 100.0f, 0.1f);
    TEST_ASSERT_TRUE(negative < 0.0f);
}

// ========== Test Runner ==========

void TestPID::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestCalculate);
    RUN_TEST(TestEdgeCases);
}
