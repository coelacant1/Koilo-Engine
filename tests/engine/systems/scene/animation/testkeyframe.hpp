/**
 * @file testkeyframe.hpp
 * @brief Unit tests for the KeyFrame class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/animation/keyframe.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKeyFrame
 * @brief Contains static test methods for the KeyFrame class.
 */
class TestKeyFrame {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSet();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
