// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaudiolistener.hpp
 * @brief Unit tests for the AudioListener class.
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
 * @class TestAudioListener
 * @brief Contains static test methods for the AudioListener class.
 */
class TestAudioListener {
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
