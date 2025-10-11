/**
 * @file testsphere.hpp
 * @brief Unit tests for the Sphere class.
 *
 * This test suite validates the Sphere class functionality including:
 * - Constructor initialization with position and radius
 * - Radius getter method
 * - Physics update with acceleration and rotation
 * - Intersection detection between spheres
 * - Collision resolution with elasticity
 * - Edge cases with zero radius, extreme values, and velocity constraints
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/3d/sphere.hpp>
#include <ptx/core/math/axisangle.hpp>
#include <ptx/core/math/rotation.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSphere
 * @brief Contains static test methods for the Sphere class.
 */
class TestSphere {
public:
    // Constructor tests
    static void TestParameterizedConstructor();

    // Getter method tests
    static void TestGetRadius();

    // Physics update tests

    // Intersection tests

    // Collision tests

    // Field access tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestCollide();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestIsIntersecting();
    static void TestUpdate();
    static void RunAllTests();
};
