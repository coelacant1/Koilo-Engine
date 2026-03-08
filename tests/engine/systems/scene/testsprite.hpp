// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsprite.hpp
 * @brief Unit tests for the Sprite class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/sprite.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSprite
 * @brief Contains static test methods for the Sprite class.
 */
class TestSprite {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetMesh();
    static void TestGetTransform();
    static void TestSetFrame();
    static void TestSetFrameIndex();
    static void TestSetHueAngle();
    static void TestSetEnabled();
    static void TestIsEnabled();
    static void TestGetMaterial();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
