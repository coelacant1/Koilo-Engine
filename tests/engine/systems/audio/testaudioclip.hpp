// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudioclip.hpp
 * @brief Unit tests for the AudioClip class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/audio/audioclip.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAudioClip
 * @brief Contains static test methods for the AudioClip class.
 */
class TestAudioClip {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestLoadFromFile();
    static void TestUnload();
    static void TestGetName();
    static void TestGetSampleRate();
    static void TestGetDuration();
    static void TestIsLoaded();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
