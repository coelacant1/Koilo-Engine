// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmorphtarget.hpp
 * @brief Unit tests for the MorphTarget class.
 *
 * @date 15/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/assets/koilomesh_loader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMorphTarget
 * @brief Contains static test methods for the MorphTarget class.
 */
class TestMorphTarget {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
