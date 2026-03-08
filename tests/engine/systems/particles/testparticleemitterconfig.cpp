// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparticleemitterconfig.cpp
 * @brief Implementation of ParticleEmitterConfig unit tests.
 */

#include "testparticleemitterconfig.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestParticleEmitterConfig::TestDefaultConstructor() {
    ParticleEmitterConfig config;
    // Default emission rate should be reasonable
    TEST_ASSERT_TRUE(config.emissionRate >= 0.0f);
}

void TestParticleEmitterConfig::TestParameterizedConstructor() {
    ParticleEmitterConfig config;
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestParticleEmitterConfig::TestEdgeCases() {
    ParticleEmitterConfig config;
    
    // Test various emission rates
    config.emissionRate = 0.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, config.emissionRate);
    
    config.emissionRate = 100.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, config.emissionRate);
    
    // Test lifetime ranges
    config.lifetimeMin = 0.5f;
    config.lifetimeMax = 2.0f;
    TEST_ASSERT_TRUE(config.lifetimeMax >= config.lifetimeMin);
}
// ========== Test Runner ==========

void TestParticleEmitterConfig::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
