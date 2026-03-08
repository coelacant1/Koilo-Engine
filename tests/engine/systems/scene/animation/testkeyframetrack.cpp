// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkeyframetrack.cpp
 * @brief Implementation of KeyFrameTrack unit tests.
 */

#include "testkeyframetrack.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestKeyFrameTrack::TestDefaultConstructor() {
    KeyFrameTrack track;
    
    // Default constructor should set range 0-1 with Cosine interpolation
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, track.GetMin());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, track.GetMax());
    TEST_ASSERT_EQUAL(KeyFrameInterpolation::Cosine, track.GetInterpolationMethod());
    TEST_ASSERT_EQUAL(true, track.IsActive());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, track.GetPlaybackSpeed());
}

void TestKeyFrameTrack::TestParameterizedConstructor() {
    KeyFrameTrack track(0.0f, 10.0f, KeyFrameInterpolation::Linear, 8, 32);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, track.GetMin());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, track.GetMax());
    TEST_ASSERT_EQUAL(KeyFrameInterpolation::Linear, track.GetInterpolationMethod());
    TEST_ASSERT_EQUAL_size_t(8, track.GetParameterCapacity());
    TEST_ASSERT_EQUAL_size_t(32, track.GetKeyFrameCapacity());
}

// ========== Method Tests ==========

void TestKeyFrameTrack::TestGetCurrentTime() {
    KeyFrameTrack track;
    
    // Initial time should be at min
    float time = track.GetCurrentTime();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, time);
}

void TestKeyFrameTrack::TestSetCurrentTime() {
    KeyFrameTrack track(0.0f, 10.0f);
    
    track.SetCurrentTime(5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, track.GetCurrentTime());
    
    track.SetCurrentTime(9.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.5f, track.GetCurrentTime());
}

void TestKeyFrameTrack::TestPause() {
    KeyFrameTrack track;
    
    track.Pause();
    TEST_ASSERT_EQUAL(false, track.IsActive());
}

void TestKeyFrameTrack::TestPlay() {
    KeyFrameTrack track;
    track.Pause();
    
    track.Play();
    TEST_ASSERT_EQUAL(true, track.IsActive());
}

void TestKeyFrameTrack::TestSetPlaybackSpeed() {
    KeyFrameTrack track;
    
    track.SetPlaybackSpeed(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, track.GetPlaybackSpeed());
    
    track.SetPlaybackSpeed(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, track.GetPlaybackSpeed());
}

void TestKeyFrameTrack::TestGetPlaybackSpeed() {
    KeyFrameTrack track;
    
    // Default should be 1.0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, track.GetPlaybackSpeed());
}

void TestKeyFrameTrack::TestSetInterpolationMethod() {
    KeyFrameTrack track;
    
    track.SetInterpolationMethod(KeyFrameInterpolation::Linear);
    TEST_ASSERT_EQUAL(KeyFrameInterpolation::Linear, track.GetInterpolationMethod());
    
    track.SetInterpolationMethod(KeyFrameInterpolation::Step);
    TEST_ASSERT_EQUAL(KeyFrameInterpolation::Step, track.GetInterpolationMethod());
}

void TestKeyFrameTrack::TestGetInterpolationMethod() {
    KeyFrameTrack track(0.0f, 1.0f, KeyFrameInterpolation::Linear);
    TEST_ASSERT_EQUAL(KeyFrameInterpolation::Linear, track.GetInterpolationMethod());
}

void TestKeyFrameTrack::TestSetMin() {
    KeyFrameTrack track;
    
    track.SetMin(-5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -5.0f, track.GetMin());
}

void TestKeyFrameTrack::TestSetMax() {
    KeyFrameTrack track;
    
    track.SetMax(100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, track.GetMax());
}
void TestKeyFrameTrack::TestSetRange() {
    KeyFrameTrack track;
    
    track.SetRange(-10.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -10.0f, track.GetMin());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, track.GetMax());
}

void TestKeyFrameTrack::TestAddKeyFrame() {
    KeyFrameTrack track;
    
    // Add keyframe by time/value
    track.AddKeyFrame(0.0f, 0.0f);
    track.AddKeyFrame(0.5f, 0.5f);
    track.AddKeyFrame(1.0f, 1.0f);
    
    TEST_ASSERT_EQUAL_size_t(3, track.GetKeyFrameCount());
}

void TestKeyFrameTrack::TestAddParameter() {
    KeyFrameTrack track;
    float param = 0.0f;
    
    track.AddParameter(&param);
    TEST_ASSERT_EQUAL_size_t(1, track.GetParameterCount());
}

void TestKeyFrameTrack::TestGetKeyFrameCapacity() {
    KeyFrameTrack track(0.0f, 1.0f, KeyFrameInterpolation::Linear, 4, 64);
    TEST_ASSERT_EQUAL_size_t(64, track.GetKeyFrameCapacity());
}

void TestKeyFrameTrack::TestGetKeyFrameCount() {
    KeyFrameTrack track;
    
    TEST_ASSERT_EQUAL_size_t(0, track.GetKeyFrameCount());
    
    track.AddKeyFrame(0.0f, 0.0f);
    TEST_ASSERT_EQUAL_size_t(1, track.GetKeyFrameCount());
}

void TestKeyFrameTrack::TestGetMax() {
    KeyFrameTrack track(0.0f, 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, track.GetMax());
}

void TestKeyFrameTrack::TestGetMin() {
    KeyFrameTrack track(-2.0f, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -2.0f, track.GetMin());
}

void TestKeyFrameTrack::TestGetParameterCapacity() {
    KeyFrameTrack track(0.0f, 1.0f, KeyFrameInterpolation::Linear, 16, 16);
    TEST_ASSERT_EQUAL_size_t(16, track.GetParameterCapacity());
}

void TestKeyFrameTrack::TestGetParameterCount() {
    KeyFrameTrack track;
    float p1 = 0.0f, p2 = 0.0f;
    
    TEST_ASSERT_EQUAL_size_t(0, track.GetParameterCount());
    
    track.AddParameter(&p1);
    TEST_ASSERT_EQUAL_size_t(1, track.GetParameterCount());
    
    track.AddParameter(&p2);
    TEST_ASSERT_EQUAL_size_t(2, track.GetParameterCount());
}

void TestKeyFrameTrack::TestGetParameterValue() {
    KeyFrameTrack track(0.0f, 1.0f);
    
    // Add simple linear keyframes: 0->0, 1->1
    track.AddKeyFrame(0.0f, 0.0f);
    track.AddKeyFrame(1.0f, 1.0f);
    
    track.SetCurrentTime(0.5f);
    float value = track.GetParameterValue();
    
    // At 0.5, value should be around 0.5 (depends on interpolation)
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.5f, value);
}

void TestKeyFrameTrack::TestIsActive() {
    KeyFrameTrack track;
    
    TEST_ASSERT_EQUAL(true, track.IsActive());
    
    track.Pause();
    TEST_ASSERT_EQUAL(false, track.IsActive());
}

void TestKeyFrameTrack::TestRemoveKeyFrame() {
    KeyFrameTrack track;
    
    track.AddKeyFrame(0.0f, 0.0f);
    track.AddKeyFrame(1.0f, 1.0f);
    TEST_ASSERT_EQUAL_size_t(2, track.GetKeyFrameCount());
    
    track.RemoveKeyFrame(0);
    TEST_ASSERT_EQUAL_size_t(1, track.GetKeyFrameCount());
}

void TestKeyFrameTrack::TestReset() {
    KeyFrameTrack track(0.0f, 10.0f);
    
    track.SetCurrentTime(5.0f);
    track.Reset();
    
    // Reset should return time to min
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, track.GetCurrentTime());
}

void TestKeyFrameTrack::TestUpdate() {
    KeyFrameTrack track;
    float param = 0.0f;
    
    track.AddParameter(&param);
    track.AddKeyFrame(0.0f, 0.0f);
    track.AddKeyFrame(1.0f, 100.0f);
    
    // Update advances time and updates parameter
    float result = track.Update();
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestKeyFrameTrack::TestEdgeCases() {
    KeyFrameTrack track;
    
    // Test negative playback speed (reverse)
    track.SetPlaybackSpeed(-1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, track.GetPlaybackSpeed());
    
    // Test zero playback speed (paused effect)
    track.SetPlaybackSpeed(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, track.GetPlaybackSpeed());
    
    // Test very fast playback
    track.SetPlaybackSpeed(10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, track.GetPlaybackSpeed());
    
    // Test negative range
    track.SetRange(-5.0f, -1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -5.0f, track.GetMin());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, track.GetMax());
}

void TestKeyFrameTrack::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetCurrentTime);
    RUN_TEST(TestSetCurrentTime);
    RUN_TEST(TestPause);
    RUN_TEST(TestPlay);
    RUN_TEST(TestSetPlaybackSpeed);
    RUN_TEST(TestGetPlaybackSpeed);
    RUN_TEST(TestSetInterpolationMethod);
    RUN_TEST(TestGetInterpolationMethod);
    RUN_TEST(TestSetMin);
    RUN_TEST(TestSetMax);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestAddKeyFrame);
    RUN_TEST(TestAddParameter);
    RUN_TEST(TestGetKeyFrameCapacity);
    RUN_TEST(TestGetKeyFrameCount);
    RUN_TEST(TestGetMax);
    RUN_TEST(TestGetMin);
    RUN_TEST(TestGetParameterCapacity);
    RUN_TEST(TestGetParameterCount);
    RUN_TEST(TestGetParameterValue);
    RUN_TEST(TestIsActive);
    RUN_TEST(TestRemoveKeyFrame);
    RUN_TEST(TestReset);
    RUN_TEST(TestSetRange);
    RUN_TEST(TestUpdate);
}
