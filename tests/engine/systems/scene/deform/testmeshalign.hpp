/**
 * @file testmeshalign.hpp
 * @brief Unit tests for the MeshAlign class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/deform/meshalign.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMeshAlign
 * @brief Contains static test methods for the MeshAlign class.
 */
class TestMeshAlign {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCentroid();
    static void TestGetObjectCenter();
    static void TestGetObjectSize();
    static void TestGetPlaneNormal();
    static void TestGetPlaneOrientation();
    static void TestGetTransform();
    static void TestGetObjectPlanarityRatio();
    static void TestSetPlaneOffsetAngle();
    static void TestSetEdgeMargin();
    static void TestSetForwardVector();
    // ... add tests for remaining 10 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestAlignObject();
    static void TestAlignObjectNoScale();
    static void TestAlignObjects();
    static void TestAlignObjectsNoScale();
    static void TestSetCameraMax();
    static void TestSetCameraMin();
    static void TestSetJustification();
    static void TestSetMirrorX();
    static void TestSetMirrorY();
    static void TestSetScale();
    static void RunAllTests();
};
