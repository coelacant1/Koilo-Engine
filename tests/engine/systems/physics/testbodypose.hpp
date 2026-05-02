/**
 * @file testbodypose.hpp
 * @brief Unit tests for the BodyPose class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/bodypose.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBodyPose
 * @brief Contains static test methods for the BodyPose class.
 */
class TestBodyPose {
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
