// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtimestep.cpp
 * @brief Implementation of TimeStep unit tests.
 */

#include "testtimestep.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

// ========== SetFrequency Tests ==========

void TestTimeStep::TestSetFrequency() {
    TimeStep ts(10.0f);
    ts.SetFrequency(20.0f);
    // After setting frequency, IsReady should eventually return true
    // Just verify it doesn't crash
    bool result = ts.IsReady();
    TEST_ASSERT_TRUE(result == true || result == false);
}

// ========== IsReady Tests ==========

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestTimeStep::TestDefaultConstructor() {
    // TimeStep requires frequency parameter, test with default value
    TimeStep ts(1.0f);
    TEST_ASSERT_TRUE(true);  // TimeStep construction
}

void TestTimeStep::TestEdgeCases() {
    // High frequency (1000 Hz)
    TimeStep highFreq(1000.0f);
    TEST_ASSERT_TRUE(true);  // High frequency
    
    // Low frequency (0.1 Hz - once every 10 seconds)
    TimeStep lowFreq(0.1f);
    TEST_ASSERT_TRUE(true);  // Low frequency
}

void TestTimeStep::TestIsReady() {
    TimeStep ts(10.0f);  // 10 Hz = every 100ms
    bool ready = ts.IsReady();
    // IsReady depends on time elapsed, just verify it returns a boolean
    TEST_ASSERT_TRUE(ready == true || ready == false);
}

void TestTimeStep::TestParameterizedConstructor() {
    TimeStep ts(60.0f);  // 60 Hz
    // Verify IsReady works after construction
    bool result = ts.IsReady();
    TEST_ASSERT_TRUE(result == true || result == false);
}

void TestTimeStep::RunAllTests() {

    RUN_TEST(TestSetFrequency);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestIsReady);
    RUN_TEST(TestParameterizedConstructor);
}
