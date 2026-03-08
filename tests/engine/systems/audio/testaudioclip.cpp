// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudioclip.cpp
 * @brief Implementation of AudioClip unit tests.
 */

#include "testaudioclip.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestAudioClip::TestDefaultConstructor() {
    AudioClip clip;
    TEST_ASSERT_FALSE(clip.IsLoaded());
    TEST_ASSERT_EQUAL_STRING("", clip.GetName().c_str());
    TEST_ASSERT_EQUAL_INT(44100, clip.GetSampleRate());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, clip.GetDuration());
}

void TestAudioClip::TestParameterizedConstructor() {
    AudioClip clip("TestSound");
    // Named constructor - not loaded yet
    TEST_ASSERT_FALSE(clip.IsLoaded());
    TEST_ASSERT_EQUAL_STRING("TestSound", clip.GetName().c_str());
}

// ========== Method Tests ==========

void TestAudioClip::TestLoadFromFile() {
    AudioClip clip;
    // Load from non-existent file should fail
    bool result = clip.LoadFromFile("nonexistent.wav");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(clip.IsLoaded());
}

void TestAudioClip::TestUnload() {
    AudioClip clip("TestClip");
    // Unload should work even if not loaded
    clip.Unload();
    TEST_ASSERT_FALSE(clip.IsLoaded());
}

void TestAudioClip::TestGetName() {
    AudioClip clip("MyAudio");
    TEST_ASSERT_EQUAL_STRING("MyAudio", clip.GetName().c_str());
    
    clip.SetName("NewName");
    TEST_ASSERT_EQUAL_STRING("NewName", clip.GetName().c_str());
}

void TestAudioClip::TestGetSampleRate() {
    AudioClip clip;
    TEST_ASSERT_EQUAL_INT(44100, clip.GetSampleRate());
}

void TestAudioClip::TestGetDuration() {
    AudioClip clip;
    // Default duration should be 0 (not loaded)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, clip.GetDuration());
}

void TestAudioClip::TestIsLoaded() {
    AudioClip clip;
    // Initially not loaded
    TEST_ASSERT_FALSE(clip.IsLoaded());
}

// ========== Edge Cases ==========

void TestAudioClip::TestEdgeCases() {
    // Empty name
    AudioClip clip1("");
    TEST_ASSERT_EQUAL_STRING("", clip1.GetName().c_str());
    
    // Very long name
    std::string longName(1000, 'a');
    AudioClip clip2(longName);
    TEST_ASSERT_EQUAL_STRING(longName.c_str(), clip2.GetName().c_str());
    
    // Load from null/empty path
    AudioClip clip3;
    TEST_ASSERT_FALSE(clip3.LoadFromFile(""));
    
    // Multiple unload calls
    AudioClip clip4;
    clip4.Unload();
    clip4.Unload();
    TEST_ASSERT_FALSE(clip4.IsLoaded());
}

// ========== Test Runner ==========

void TestAudioClip::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestLoadFromFile);
    RUN_TEST(TestUnload);
    RUN_TEST(TestGetName);
    RUN_TEST(TestGetSampleRate);
    RUN_TEST(TestGetDuration);
    RUN_TEST(TestIsLoaded);
    RUN_TEST(TestEdgeCases);
}
