/**
 * @file testheightfieldcollider.hpp
 * @brief Unit tests for the HeightfieldCollider class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/heightfieldcollider.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestHeightfieldCollider
 * @brief Contains static test methods for the HeightfieldCollider class.
 */
class TestHeightfieldCollider {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPosition();
    static void TestSetPosition();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
