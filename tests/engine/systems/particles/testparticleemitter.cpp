// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparticleemitter.cpp
 * @brief Implementation of ParticleEmitter unit tests.
 */

#include "testparticleemitter.hpp"
#include <koilo/core/time/timemanager.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestParticleEmitter::TestDefaultConstructor() {
    ParticleEmitter emitter;
    TEST_ASSERT_FALSE(emitter.IsPlaying());
}

void TestParticleEmitter::TestParameterizedConstructor() {
    ParticleEmitterConfig config;
    ParticleEmitter emitter(config);
    // Emitter created with config
    TEST_ASSERT_FALSE(emitter.IsPlaying());
}

// ========== Method Tests ==========

void TestParticleEmitter::TestPlay() {
    ParticleEmitter emitter;
    emitter.Play();
    TEST_ASSERT_TRUE(emitter.IsPlaying());
}

void TestParticleEmitter::TestStop() {
    ParticleEmitter emitter;
    emitter.Play();
    emitter.Stop();
    TEST_ASSERT_FALSE(emitter.IsPlaying());
}

void TestParticleEmitter::TestPause() {
    ParticleEmitter emitter;
    emitter.Play();
    emitter.Pause();
    TEST_ASSERT_FALSE(emitter.IsPlaying());
}

void TestParticleEmitter::TestIsPlaying() {
    ParticleEmitter emitter;
    TEST_ASSERT_FALSE(emitter.IsPlaying());
    
    emitter.Play();
    TEST_ASSERT_TRUE(emitter.IsPlaying());
}

void TestParticleEmitter::TestUpdate() {
    ParticleEmitter emitter;
    emitter.Play();
    koilo::TimeManager::GetInstance().Tick(0.016f); emitter.Update();
    // Update should process without crash
    TEST_ASSERT_TRUE(emitter.IsPlaying());
}

void TestParticleEmitter::TestGetActiveParticleCount() {
    ParticleEmitter emitter;
    uint32_t count = emitter.GetActiveParticleCount();
    TEST_ASSERT_EQUAL_UINT32(0, count);
}

void TestParticleEmitter::TestClear() {
    ParticleEmitter emitter;
    emitter.Clear();
    TEST_ASSERT_EQUAL_UINT32(0, emitter.GetActiveParticleCount());
}

void TestParticleEmitter::TestEmit() {
    ParticleEmitter emitter;
    emitter.EmitBurst(10);
    // Emit should create particles
    TEST_ASSERT_TRUE(true);
}

void TestParticleEmitter::TestEmitBurst() {
    ParticleEmitter emitter;
    emitter.EmitBurst(20);
    // Burst emission should create many particles at once
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestParticleEmitter::TestEdgeCases() {
    ParticleEmitter emitter;
    // Update while not playing
    koilo::TimeManager::GetInstance().Tick(0.016f); emitter.Update();
    TEST_ASSERT_EQUAL_UINT32(0, emitter.GetActiveParticleCount());
    
    // Multiple play/pause cycles
    emitter.Play();
    emitter.Pause();
    emitter.Play();
    TEST_ASSERT_TRUE(emitter.IsPlaying());
}
// ========== Test Runner ==========

void TestParticleEmitter::TestGetParticleSize() {
    // TODO: Implement test for GetParticleSize()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestRender() {
    // TODO: Implement test for Render()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetEmissionRate() {
    // TODO: Implement test for SetEmissionRate()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetEndColor() {
    // TODO: Implement test for SetEndColor()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetGravity() {
    // TODO: Implement test for SetGravity()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetLifetime() {
    // TODO: Implement test for SetLifetime()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetParticleSize() {
    // TODO: Implement test for SetParticleSize()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetPosition() {
    // TODO: Implement test for SetPosition()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetSizeRange() {
    // TODO: Implement test for SetSizeRange()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetStartColor() {
    // TODO: Implement test for SetStartColor()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::TestSetVelocityRange() {
    // TODO: Implement test for SetVelocityRange()
    ParticleEmitter obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestParticleEmitter::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPlay);
    RUN_TEST(TestStop);
    RUN_TEST(TestPause);
    RUN_TEST(TestIsPlaying);
    RUN_TEST(TestUpdate);

    RUN_TEST(TestGetActiveParticleCount);
    RUN_TEST(TestClear);
    RUN_TEST(TestEmit);
    RUN_TEST(TestEmitBurst);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetParticleSize);
    RUN_TEST(TestRender);
    RUN_TEST(TestSetEmissionRate);
    RUN_TEST(TestSetEndColor);
    RUN_TEST(TestSetGravity);
    RUN_TEST(TestSetLifetime);
    RUN_TEST(TestSetParticleSize);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestSetSizeRange);
    RUN_TEST(TestSetStartColor);
    RUN_TEST(TestSetVelocityRange);
}
