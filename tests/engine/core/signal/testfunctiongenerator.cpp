/**
 * @file testfunctiongenerator.cpp
 * @brief Implementation of FunctionGenerator unit tests.
 */

#include "testfunctiongenerator.hpp"

// ========== Constructor Tests ==========

void TestFunctionGenerator::TestDefaultConstructor() {
    // FunctionGenerator has no default constructor - requires 4 parameters
    // Test with minimal parameters
    FunctionGenerator generator(FunctionGenerator::Sine, 0.0f, 1.0f, 1000.0f);

    // Verify generator can be constructed and updated
    float value = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(value));
    TEST_ASSERT_TRUE(value >= 0.0f && value <= 1.0f);
}

// ========== Method Tests ==========
void TestFunctionGenerator::TestSetPeriod() {
    FunctionGenerator generator(FunctionGenerator::Sine, 0.0f, 1.0f, 1000.0f);

    // Change the period
    generator.SetPeriod(500.0f);
    float value1 = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(value1));

    // Change again
    generator.SetPeriod(2000.0f);
    float value2 = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(value2));

    // Test with very small period
    generator.SetPeriod(10.0f);
    float value3 = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(value3));

    // Test with very large period
    generator.SetPeriod(10000.0f);
    float value4 = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(value4));
}

void TestFunctionGenerator::TestSetFunction() {
    FunctionGenerator generator(FunctionGenerator::Sine, 0.0f, 1.0f, 1000.0f);

    // Test all function types
    generator.SetFunction(FunctionGenerator::Triangle);
    float triangle = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(triangle));

    generator.SetFunction(FunctionGenerator::Square);
    float square = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(square));

    generator.SetFunction(FunctionGenerator::Sine);
    float sine = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(sine));

    generator.SetFunction(FunctionGenerator::Sawtooth);
    float sawtooth = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(sawtooth));

    generator.SetFunction(FunctionGenerator::Gravity);
    float gravity = generator.Update();
    TEST_ASSERT_TRUE(std::isfinite(gravity));

    // All values should be within the min/max range
    TEST_ASSERT_TRUE(triangle >= 0.0f && triangle <= 1.0f);
    TEST_ASSERT_TRUE(square >= 0.0f && square <= 1.0f);
    TEST_ASSERT_TRUE(sine >= 0.0f && sine <= 1.0f);
    TEST_ASSERT_TRUE(sawtooth >= 0.0f && sawtooth <= 1.0f);
    TEST_ASSERT_TRUE(gravity >= 0.0f && gravity <= 1.0f);
}

void TestFunctionGenerator::TestUpdate() {
    FunctionGenerator generator(FunctionGenerator::Sine, 0.0f, 1.0f, 1000.0f);

    // Call Update multiple times
    for (int i = 0; i < 100; i++) {
        float value = generator.Update();
        TEST_ASSERT_TRUE(std::isfinite(value));
        TEST_ASSERT_TRUE(value >= 0.0f && value <= 1.0f);
    }

    // Test with different min/max ranges
    FunctionGenerator gen2(FunctionGenerator::Triangle, -10.0f, 10.0f, 500.0f);
    for (int i = 0; i < 50; i++) {
        float value = gen2.Update();
        TEST_ASSERT_TRUE(std::isfinite(value));
        TEST_ASSERT_TRUE(value >= -10.0f && value <= 10.0f);
    }

    // Test with negative range
    FunctionGenerator gen3(FunctionGenerator::Square, -1.0f, 0.0f, 200.0f);
    for (int i = 0; i < 20; i++) {
        float value = gen3.Update();
        TEST_ASSERT_TRUE(std::isfinite(value));
        TEST_ASSERT_TRUE(value >= -1.0f && value <= 0.0f);
    }
}

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestFunctionGenerator::TestParameterizedConstructor() {
    // Test with all function types
    FunctionGenerator gen1(FunctionGenerator::Triangle, 0.0f, 100.0f, 1000.0f);
    float val1 = gen1.Update();
    TEST_ASSERT_TRUE(std::isfinite(val1));
    TEST_ASSERT_TRUE(val1 >= 0.0f && val1 <= 100.0f);

    FunctionGenerator gen2(FunctionGenerator::Square, -50.0f, 50.0f, 500.0f);
    float val2 = gen2.Update();
    TEST_ASSERT_TRUE(std::isfinite(val2));
    TEST_ASSERT_TRUE(val2 >= -50.0f && val2 <= 50.0f);

    FunctionGenerator gen3(FunctionGenerator::Sawtooth, 10.0f, 20.0f, 2000.0f);
    float val3 = gen3.Update();
    TEST_ASSERT_TRUE(std::isfinite(val3));
    TEST_ASSERT_TRUE(val3 >= 10.0f && val3 <= 20.0f);

    FunctionGenerator gen4(FunctionGenerator::Gravity, 0.0f, 1.0f, 100.0f);
    float val4 = gen4.Update();
    TEST_ASSERT_TRUE(std::isfinite(val4));
    TEST_ASSERT_TRUE(val4 >= 0.0f && val4 <= 1.0f);
}

void TestFunctionGenerator::TestEdgeCases() {
    // Test with zero range (min == max)
    FunctionGenerator gen1(FunctionGenerator::Sine, 5.0f, 5.0f, 1000.0f);
    float zeroRange = gen1.Update();
    TEST_ASSERT_TRUE(std::isfinite(zeroRange));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5.0f, zeroRange);

    // Test with inverted range (max < min)
    FunctionGenerator gen2(FunctionGenerator::Triangle, 10.0f, 0.0f, 500.0f);
    float inverted = gen2.Update();
    TEST_ASSERT_TRUE(std::isfinite(inverted));

    // Test with very small period
    FunctionGenerator gen3(FunctionGenerator::Square, 0.0f, 1.0f, 1.0f);
    for (int i = 0; i < 10; i++) {
        float val = gen3.Update();
        TEST_ASSERT_TRUE(std::isfinite(val));
    }

    // Test with very large period
    FunctionGenerator gen4(FunctionGenerator::Sawtooth, 0.0f, 1.0f, 1000000.0f);
    for (int i = 0; i < 10; i++) {
        float val = gen4.Update();
        TEST_ASSERT_TRUE(std::isfinite(val));
    }

    // Test with large value range
    FunctionGenerator gen5(FunctionGenerator::Sine, -1000.0f, 1000.0f, 500.0f);
    float largeRange = gen5.Update();
    TEST_ASSERT_TRUE(std::isfinite(largeRange));
    TEST_ASSERT_TRUE(largeRange >= -1000.0f && largeRange <= 1000.0f);

    // Test function switching during operation
    FunctionGenerator gen6(FunctionGenerator::Sine, 0.0f, 1.0f, 1000.0f);
    gen6.Update();
    gen6.Update();
    gen6.SetFunction(FunctionGenerator::Triangle);
    float switched = gen6.Update();
    TEST_ASSERT_TRUE(std::isfinite(switched));

    // Test period switching during operation
    gen6.SetPeriod(100.0f);
    float periodSwitched = gen6.Update();
    TEST_ASSERT_TRUE(std::isfinite(periodSwitched));

    // Test all functions produce values in range
    for (int funcType = FunctionGenerator::Triangle; funcType <= FunctionGenerator::Gravity; funcType++) {
        FunctionGenerator gen(static_cast<FunctionGenerator::Function>(funcType), -5.0f, 5.0f, 500.0f);

        for (int i = 0; i < 20; i++) {
            float val = gen.Update();
            TEST_ASSERT_TRUE(std::isfinite(val));
            // Allow some tolerance for floating point edge cases
            TEST_ASSERT_TRUE(val >= -5.5f && val <= 5.5f);
        }
    }

    // Test negative period (edge case behavior)
    FunctionGenerator gen7(FunctionGenerator::Sine, 0.0f, 1.0f, 1000.0f);
    gen7.SetPeriod(-500.0f);  // Negative period
    float negPeriod = gen7.Update();
    TEST_ASSERT_TRUE(std::isfinite(negPeriod));

    // Test zero period (edge case behavior)
    gen7.SetPeriod(0.0f);
    float zeroPeriod = gen7.Update();
    TEST_ASSERT_TRUE(std::isfinite(zeroPeriod));
}

void TestFunctionGenerator::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetPeriod);
    RUN_TEST(TestSetFunction);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestEdgeCases);
}
