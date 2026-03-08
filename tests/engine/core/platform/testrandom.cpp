// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrandom.cpp
 * @brief Implementation of Random tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testrandom.hpp"
#include <koilo/core/platform/random.hpp>

using namespace koilo;

void TestRandom::TestSeed() {
    Random::Seed(12345);
    int val = Random::Int(0, 100);
    TEST_ASSERT_TRUE(val >= 0 && val <= 100);
}

void TestRandom::TestIntRange() {
    Random::Seed(42);
    
    for (int i = 0; i < 100; i++) {
        int value = Random::Int(0, 10);
        TEST_ASSERT_TRUE(value >= 0);
        TEST_ASSERT_TRUE(value <= 10);
    }
}

void TestRandom::TestIntMinMax() {
    Random::Seed(42);
    
    int value = Random::Int(5, 5);
    TEST_ASSERT_EQUAL(5, value);
}

void TestRandom::TestIntSameSeed() {
    Random::Seed(42);
    int val1 = Random::Int(0, 100);
    
    Random::Seed(42);
    int val2 = Random::Int(0, 100);
    
    TEST_ASSERT_EQUAL(val1, val2);
}

void TestRandom::TestFloatRange() {
    Random::Seed(42);
    
    for (int i = 0; i < 100; i++) {
        float value = Random::Float(0.0f, 1.0f);
        TEST_ASSERT_TRUE(value >= 0.0f);
        TEST_ASSERT_TRUE(value < 1.0f);
    }
}

void TestRandom::TestFloatMinMax() {
    Random::Seed(42);
    
    for (int i = 0; i < 100; i++) {
        float value = Random::Float(5.0f, 10.0f);
        TEST_ASSERT_TRUE(value >= 5.0f);
        TEST_ASSERT_TRUE(value < 10.0f);
    }
}

void TestRandom::TestFloatDistribution() {
    Random::Seed(42);
    
    int lowCount = 0;
    int highCount = 0;
    
    for (int i = 0; i < 100; i++) {
        float value = Random::Float(0.0f, 1.0f);
        if (value < 0.5f) lowCount++;
        else highCount++;
    }
    
    // Should have roughly even distribution
    TEST_ASSERT_TRUE(lowCount > 20);
    TEST_ASSERT_TRUE(highCount > 20);
}

void TestRandom::TestMultipleValues() {
    Random::Seed(42);
    
    int val1 = Random::Int(0, 100);
    int val2 = Random::Int(0, 100);
    
    // Different calls should give different values (statistically)
    // This might occasionally fail, but very unlikely
    bool allSame = true;
    for (int i = 0; i < 10; i++) {
        if (Random::Int(0, 100) != val1) {
            allSame = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(allSame);
}

void TestRandom::RunAllTests() {
    RUN_TEST(TestSeed);
    RUN_TEST(TestIntRange);
    RUN_TEST(TestIntMinMax);
    RUN_TEST(TestIntSameSeed);
    RUN_TEST(TestFloatRange);
    RUN_TEST(TestFloatMinMax);
    RUN_TEST(TestFloatDistribution);
    RUN_TEST(TestMultipleValues);
}
