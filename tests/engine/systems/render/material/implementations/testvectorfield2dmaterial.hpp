/**
 * @file testvectorfield2dmaterial.hpp
 * @brief Unit tests for the VectorField2DMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/vectorfield2dmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVectorField2DMaterial
 * @brief Contains static test methods for the VectorField2DMaterial class.
 */
class TestVectorField2DMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetSize();
    static void TestSetPosition();
    static void TestSetRotation();
    static void TestRenderDensity();
    static void TestRenderVector();
    static void TestGetCountX();
    static void TestGetCountY();
    static void TestBoundary();
    static void TestDiffuse();
    static void TestAdvect();
    // ... add tests for remaining 4 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestMovingSquareField();
    static void TestSineField();
    static void TestSpiralField();
    static void TestStepField();
    static void RunAllTests();
};
