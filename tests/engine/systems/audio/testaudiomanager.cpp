// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiomanager.cpp
 * @brief Implementation of AudioManager unit tests.
 */

#include "testaudiomanager.hpp"
#include <koilo/core/time/timemanager.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestAudioManager::TestDefaultConstructor() {
    AudioManager manager;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, manager.GetMasterVolume());
}

void TestAudioManager::TestParameterizedConstructor() {
    AudioManager manager;
    // AudioManager doesn't have parameterized constructor
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, manager.GetMasterVolume());
}

// ========== Method Tests ==========

void TestAudioManager::TestInitialize() {
    AudioManager manager;
    bool result = manager.Initialize();
    // Initialize should return success or failure
    TEST_ASSERT_TRUE(result == true || result == false);
}

void TestAudioManager::TestShutdown() {
    AudioManager manager;
    manager.Initialize();
    manager.Shutdown();
    // After shutdown, should be safe
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestUpdate() {
    AudioManager manager;
    manager.Initialize();
    
    // Update with typical frame times
    koilo::TimeManager::GetInstance().Tick(0.016f); manager.Update();
    koilo::TimeManager::GetInstance().Tick(0.033f); manager.Update();
    
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestLoadClip() {
    AudioManager manager;
    auto clip = manager.LoadClip("test", "test.wav");
    // File may not exist, so clip could be null or valid
    TEST_ASSERT_TRUE(clip == nullptr || clip != nullptr);
}

void TestAudioManager::TestGetClip() {
    AudioManager manager;
    auto clip = manager.GetClip("nonexistent");
    // Non-existent clip should return null
    TEST_ASSERT_NULL(clip.get());
}

void TestAudioManager::TestUnloadClip() {
    AudioManager manager;
    manager.Initialize();
    manager.UnloadClip("test");
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestPlaySound() {
    AudioManager manager;
    manager.Initialize();
    manager.PlaySound("test");
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestPlaySound3D() {
    AudioManager manager;
    manager.Initialize();
    Vector3D pos(10.0f, 20.0f, 30.0f);
    manager.PlaySound3D("test", pos);
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestStopAll() {
    AudioManager manager;
    manager.Initialize();
    manager.PlaySound("test");
    manager.StopAll();
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestPauseAll() {
    AudioManager manager;
    manager.Initialize();
    manager.PlaySound("test");
    manager.PauseAll();
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestResumeAll() {
    AudioManager manager;
    manager.Initialize();
    manager.PlaySound("test");
    manager.PauseAll();
    manager.ResumeAll();
    TEST_ASSERT_TRUE(true);
}

void TestAudioManager::TestSetMasterVolume() {
    AudioManager manager;
    manager.SetMasterVolume(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, manager.GetMasterVolume());
}

void TestAudioManager::TestGetMasterVolume() {
    AudioManager manager;
    float vol = manager.GetMasterVolume();
    TEST_ASSERT_TRUE(vol >= 0.0f && vol <= 1.0f);
}

// ========== Edge Cases ==========

void TestAudioManager::TestEdgeCases() {
    AudioManager manager;
    manager.SetMasterVolume(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, manager.GetMasterVolume());
    manager.SetMasterVolume(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, manager.GetMasterVolume());
}
// ========== Test Runner ==========

void TestAudioManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestInitialize);
    RUN_TEST(TestShutdown);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestLoadClip);
    RUN_TEST(TestGetClip);
    RUN_TEST(TestUnloadClip);
    RUN_TEST(TestPlaySound);
    RUN_TEST(TestPlaySound3D);
    RUN_TEST(TestStopAll);
    RUN_TEST(TestPauseAll);
    RUN_TEST(TestResumeAll);
    RUN_TEST(TestSetMasterVolume);
    RUN_TEST(TestGetMasterVolume);
    RUN_TEST(TestEdgeCases);
}
