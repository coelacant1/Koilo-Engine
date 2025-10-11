/**
 * @file testmeshdeformer.hpp
 * @brief Unit tests for the MeshDeformer class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/deform/meshdeformer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMeshDeformer
 * @brief Contains static test methods for the MeshDeformer class.
 */
class TestMeshDeformer {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestPerspectiveDeform();
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
