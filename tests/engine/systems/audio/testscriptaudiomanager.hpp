// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptaudiomanager.hpp
 * @brief Unit tests for the ScriptAudioManager class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/audio/script_audio_manager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestScriptAudioManager
 * @brief Contains static test methods for the ScriptAudioManager class.
 */
class TestScriptAudioManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
