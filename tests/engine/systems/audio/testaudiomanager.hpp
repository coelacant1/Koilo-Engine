// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiomanager.hpp
 * @brief Unit tests for the AudioManager class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/audio/audiomanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAudioManager
 * @brief Contains static test methods for the AudioManager class.
 */
class TestAudioManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestShutdown();
    static void TestUpdate();
    static void TestLoadClip();
    static void TestGetClip();
    static void TestUnloadClip();
    static void TestPlaySound();
    static void TestPlaySound3D();
    static void TestStopAll();
    static void TestPauseAll();
    static void TestResumeAll();
    static void TestSetMasterVolume();
    static void TestGetMasterVolume();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
