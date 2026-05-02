/**
 * @file testaudiomodule.hpp
 * @brief Unit tests for the AudioModule class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/audio/audio_module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAudioModule
 * @brief Contains static test methods for the AudioModule class.
 */
class TestAudioModule {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetInfo();
    static void TestUpdate();
    static void TestShutdown();
    static void TestGetManager();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
