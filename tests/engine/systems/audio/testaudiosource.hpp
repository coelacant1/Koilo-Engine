// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiosource.hpp
 * @brief Unit tests for the AudioSource class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/audio/audiosource.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAudioSource
 * @brief Contains static test methods for the AudioSource class.
 */
class TestAudioSource {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestPlay();
    static void TestPause();
    static void TestStop();
    static void TestIsPlaying();
    static void TestIsPaused();
    static void TestIsStopped();
    static void TestSetPosition();
    static void TestGetPosition();
    static void TestSetVolume();
    static void TestGetVolume();
    static void TestSetPitch();
    static void TestGetPitch();
    static void TestSetLoop();
    static void TestIsLooping();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
