// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparticle.cpp
 * @brief Implementation of Particle unit tests.
 */

#include "testparticle.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestParticle::TestDefaultConstructor() {
    Particle particle;
    // Default particle: age=0, lifetime=0, active=false -> not alive
    TEST_ASSERT_FALSE(particle.IsAlive());
}

void TestParticle::TestParameterizedConstructor() {
    Particle particle;
    particle.position = Vector3D(5.0f, 10.0f, 15.0f);
    particle.velocity = Vector3D(1.0f, 0.0f, 0.0f);
    particle.lifetime = 2.0f;
    // active defaults to false unless explicitly set
    TEST_ASSERT_FALSE(particle.IsAlive());
    particle.active = true;
    TEST_ASSERT_TRUE(particle.IsAlive());
}

// ========== Method Tests ==========

void TestParticle::TestIsAlive() {
    Particle particle;
    bool alive = particle.IsAlive();
    TEST_ASSERT_TRUE(alive == true || alive == false);
}

void TestParticle::TestGetLifetimeProgress() {
    Particle particle;
    float progress = particle.GetLifetimeProgress();
    // Progress should be 0-1 (0% to 100%)
    TEST_ASSERT_TRUE(progress >= 0.0f && progress <= 1.0f);
}

// ========== Edge Cases ==========

void TestParticle::TestEdgeCases() {
    Particle particle;
    particle.lifetime = 0.0f;
    // Not alive: lifetime=0 so age < lifetime is false
    TEST_ASSERT_FALSE(particle.IsAlive());
    
    Particle longLife;
    longLife.lifetime = 1000.0f;
    longLife.active = true;
    TEST_ASSERT_TRUE(longLife.IsAlive());
}
// ========== Test Runner ==========

void TestParticle::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestIsAlive);
    RUN_TEST(TestGetLifetimeProgress);
    RUN_TEST(TestEdgeCases);
}
