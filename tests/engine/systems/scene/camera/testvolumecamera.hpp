// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvolumecamera.hpp
 * @brief Tests for VolumeCamera FBO sampling and output packing.
 */
#pragma once
#include <unity.h>

class TestVolumeCamera {
public:
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestGetBrightness();
    static void TestGetCameraCenterCoordinate();
    static void TestGetCameraMaxCoordinate();
    static void TestGetCameraMinCoordinate();
    static void TestGetCameraTransformCenter();
    static void TestGetCameraTransformMax();
    static void TestGetCameraTransformMin();
    static void TestGetFBOHeight();
    static void TestGetFBOWidth();
    static void TestGetGamma();
    static void TestGetPixelCount();
    static void TestGetPixelGroup();
    static void TestParameterizedConstructor();
    static void TestSetBrightness();
    static void TestSetGamma();
    static void TestSetResolution();
    static void RunAllTests();
};
