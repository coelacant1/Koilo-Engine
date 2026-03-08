// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtexture.hpp
 * @brief Unit tests for the Texture class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/assets/image/texture.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTexture
 * @brief Contains static test methods for the Texture class.
 */
class TestTexture {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetWidth();
    static void TestGetHeight();
    static void TestSampleUV();
    static void TestSamplePixel();
    static void TestSetPixel();
    static void TestSetIndex();
    static void TestCreateRGB();
    static void TestGetPaletteSize();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
