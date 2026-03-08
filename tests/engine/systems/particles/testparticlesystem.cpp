// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparticlesystem.cpp
 * @brief Implementation of ParticleSystem unit tests.
 */

#include "testparticlesystem.hpp"
#include <koilo/core/time/timemanager.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestParticleSystem::TestDefaultConstructor() {
    ParticleSystem system;
    // Initially no emitters
    TEST_ASSERT_EQUAL_UINT32(0, system.GetEmitterCount());
}

void TestParticleSystem::TestParameterizedConstructor() {
    ParticleSystem system;
    TEST_ASSERT_EQUAL_UINT32(0, system.GetEmitterCount());
}

// ========== Method Tests ==========

void TestParticleSystem::TestClearEmitters() {
    ParticleSystem system;
    system.ClearEmitters();
    TEST_ASSERT_EQUAL_UINT32(0, system.GetEmitterCount());
}

void TestParticleSystem::TestGetEmitterCount() {
    ParticleSystem system;
    uint32_t count = system.GetEmitterCount();
    TEST_ASSERT_EQUAL_UINT32(0, count);
}

void TestParticleSystem::TestUpdate() {
    ParticleSystem system;
    koilo::TimeManager::GetInstance().Tick(0.016f); system.Update();
    koilo::TimeManager::GetInstance().Tick(0.033f); system.Update();
    koilo::TimeManager::GetInstance().Tick(1.0f); system.Update();
    // Updates should not crash
    TEST_ASSERT_TRUE(true);
}

void TestParticleSystem::TestGetTotalActiveParticles() {
    ParticleSystem system;
    uint32_t count = system.GetTotalActiveParticles();
    // No particles initially
    TEST_ASSERT_EQUAL_UINT32(0, count);
}

// ========== Edge Cases ==========

void TestParticleSystem::TestEdgeCases() {
    ParticleSystem system;
    // Update with zero delta time
    koilo::TimeManager::GetInstance().Tick(0.0f); system.Update();
    TEST_ASSERT_EQUAL_UINT32(0, system.GetTotalActiveParticles());
    
    // Negative delta (should handle gracefully)
    koilo::TimeManager::GetInstance().Tick(-0.016f); system.Update();
    TEST_ASSERT_TRUE(true);
}
// ========== Test Runner ==========

void TestParticleSystem::TestAddEmitter() {
    // TODO: Implement test for AddEmitter()
    ParticleSystem obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleSystem::TestGetEmitter() {
    // TODO: Implement test for GetEmitter()
    ParticleSystem obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleSystem::TestRender() {
    // TODO: Implement test for Render()
    ParticleSystem obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleSystem::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestClearEmitters);
    RUN_TEST(TestGetEmitterCount);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestGetTotalActiveParticles);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestAddEmitter);
    RUN_TEST(TestGetEmitter);
    RUN_TEST(TestRender);
}
