/**
 * @file testtrianglegroupdeformer.hpp
 * @brief Unit tests for the TriangleGroupDeformer class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/deform/trianglegroupdeformer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTriangleGroupDeformer
 * @brief Contains static test methods for the TriangleGroupDeformer class.
 */
class TestTriangleGroupDeformer {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSinusoidalDeform();
    static void TestDropwaveDeform();
    static void TestSineWaveSurfaceDeform();
    static void TestCosineInterpolationDeformer();
    static void TestAxisZeroClipping();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
