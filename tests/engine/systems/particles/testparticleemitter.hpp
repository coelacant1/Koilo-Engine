// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparticleemitter.hpp
 * @brief Unit tests for the ParticleEmitter class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/particles/particleemitter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestParticleEmitter
 * @brief Contains static test methods for the ParticleEmitter class.
 */
class TestParticleEmitter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestPlay();
    static void TestStop();
    static void TestPause();
    static void TestIsPlaying();
    static void TestUpdate();

    static void TestGetActiveParticleCount();
    static void TestClear();
    static void TestEmit();
    static void TestEmitBurst();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetParticleSize();
    static void TestRender();
    static void TestSetEmissionRate();
    static void TestSetEndColor();
    static void TestSetGravity();
    static void TestSetLifetime();
    static void TestSetParticleSize();
    static void TestSetPosition();
    static void TestSetSizeRange();
    static void TestSetStartColor();
    static void TestSetVelocityRange();
    static void RunAllTests();
};
