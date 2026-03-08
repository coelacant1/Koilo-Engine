// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiosource.cpp
 * @brief Implementation of AudioSource unit tests.
 */

#include "testaudiosource.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestAudioSource::TestDefaultConstructor() {
    AudioSource source;
    // Default state should be stopped
    TEST_ASSERT_TRUE(source.IsStopped());
    TEST_ASSERT_FALSE(source.IsPlaying());
    TEST_ASSERT_FALSE(source.IsPaused());
    
    // Default audio properties
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, source.GetVolume());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, source.GetPitch());
    TEST_ASSERT_FALSE(source.IsLooping());
}

void TestAudioSource::TestParameterizedConstructor() {
    auto clip = std::make_shared<AudioClip>("TestClip");
    AudioSource source(clip);
    
    // Should have the clip
    TEST_ASSERT_NOT_NULL(source.GetClip().get());
    TEST_ASSERT_TRUE(source.IsStopped());
}

// ========== Method Tests ==========

void TestAudioSource::TestPlay() {
    AudioSource source;
    source.Play();
    // Play() no-ops without a loaded clip
    TEST_ASSERT_FALSE(source.IsPlaying());
    TEST_ASSERT_TRUE(source.IsStopped());
}

void TestAudioSource::TestPause() {
    AudioSource source;
    // Without a clip, state stays stopped
    source.Pause();
    TEST_ASSERT_TRUE(source.IsStopped());
}

void TestAudioSource::TestStop() {
    AudioSource source;
    TEST_ASSERT_TRUE(source.IsStopped());
    source.Stop();
    TEST_ASSERT_TRUE(source.IsStopped());
}

void TestAudioSource::TestIsPlaying() {
    AudioSource source;
    TEST_ASSERT_FALSE(source.IsPlaying());
    // Play() requires a clip to transition state
    source.Play();
    TEST_ASSERT_FALSE(source.IsPlaying());
}

void TestAudioSource::TestIsPaused() {
    AudioSource source;
    TEST_ASSERT_FALSE(source.IsPaused());
}

void TestAudioSource::TestIsStopped() {
    AudioSource source;
    TEST_ASSERT_TRUE(source.IsStopped());
    
    // Without clip, Play() doesn't change state
    source.Play();
    TEST_ASSERT_TRUE(source.IsStopped());
}

void TestAudioSource::TestSetPosition() {
    AudioSource source;
    Vector3D pos(10.0f, 20.0f, 30.0f);
    
    source.SetPosition(pos);
    Vector3D result = source.GetPosition();
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, result.Z);
}

void TestAudioSource::TestGetPosition() {
    AudioSource source;
    Vector3D pos = source.GetPosition();
    // Default position should be origin
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, pos.Z);
}

void TestAudioSource::TestSetVolume() {
    AudioSource source;
    
    source.SetVolume(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, source.GetVolume());
    
    source.SetVolume(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, source.GetVolume());
    
    source.SetVolume(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, source.GetVolume());
}

void TestAudioSource::TestGetVolume() {
    AudioSource source;
    // Default volume should be 1.0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, source.GetVolume());
}

void TestAudioSource::TestSetPitch() {
    AudioSource source;
    
    source.SetPitch(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, source.GetPitch());
    
    source.SetPitch(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, source.GetPitch());
}

void TestAudioSource::TestGetPitch() {
    AudioSource source;
    // Default pitch should be 1.0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, source.GetPitch());
}

void TestAudioSource::TestSetLoop() {
    AudioSource source;
    
    source.SetLoop(true);
    TEST_ASSERT_TRUE(source.IsLooping());
    
    source.SetLoop(false);
    TEST_ASSERT_FALSE(source.IsLooping());
}

void TestAudioSource::TestIsLooping() {
    AudioSource source;
    // Default should not be looping
    TEST_ASSERT_FALSE(source.IsLooping());
}

// ========== Edge Cases ==========

void TestAudioSource::TestEdgeCases() {
    AudioSource source;
    
    // Volume clamping (0.0 to 1.0)
    source.SetVolume(-1.0f);  // Should clamp to 0
    TEST_ASSERT_TRUE(source.GetVolume() >= 0.0f);
    
    source.SetVolume(2.0f);  // Should clamp to 1 or allow >1
    TEST_ASSERT_TRUE(source.GetVolume() >= 0.0f);
    
    // Pitch extremes
    source.SetPitch(0.1f);  // Very slow
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, source.GetPitch());
    
    source.SetPitch(4.0f);  // Clamped to max 3.0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, source.GetPitch());
    
    // State transitions
    source.Stop();
    source.Pause();  // Pause from stopped
    TEST_ASSERT_TRUE(source.IsPaused() || source.IsStopped());
    
    // Play -> Stop -> Play (no clip, so Play() no-ops)
    source.Play();
    source.Stop();
    source.Play();
    TEST_ASSERT_TRUE(source.IsStopped());
}

// ========== Test Runner ==========

void TestAudioSource::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestPlay);
    RUN_TEST(TestPause);
    RUN_TEST(TestStop);
    RUN_TEST(TestIsPlaying);
    RUN_TEST(TestIsPaused);
    RUN_TEST(TestIsStopped);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestSetVolume);
    RUN_TEST(TestGetVolume);
    RUN_TEST(TestSetPitch);
    RUN_TEST(TestGetPitch);
    RUN_TEST(TestSetLoop);
    RUN_TEST(TestIsLooping);
    RUN_TEST(TestEdgeCases);
}
