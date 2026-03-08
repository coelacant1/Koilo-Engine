// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiobackend.hpp
 * @brief Unit tests for the AudioBackend class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/audio/audiobackend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAudioBackend
 * @brief Contains static test methods for the AudioBackend class.
 */
class TestAudioBackend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestAddSource();
    static void TestRemoveSource();
    static void TestClearSources();
    static void TestSetListenerForward();
    static void TestGetSampleRate();
    static void TestGetChannels();
    static void TestMixFrames();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
