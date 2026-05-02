// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkslmaterial.hpp
 * @brief Integration tests for KSLMaterial bridge and meta shader composition.
 */

#pragma once

#include <unity.h>
#include <koilo/ksl/ksl_module.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <utils/testhelpers.hpp>

class TestKSLMaterial {
public:
    // KSLMaterial lifecycle

    // Shading through IMaterial interface

    // Parameter setting
    static void TestSetFloat();
    static void TestSetVec3();

    // KSLModule direct tests

    static void TestAddLight();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestIsBound();
    static void TestLightCount();
    static void TestParameterizedConstructor();
    static void TestSetCameraPosition();
    static void TestSetColor();
    static void TestSetColorAt();
    static void TestSetDeltaTime();
    static void TestSetInt();
    static void TestSetLightColor();
    static void TestSetLightCurve();
    static void TestSetLightFalloff();
    static void TestSetLightIntensity();
    static void TestSetLightPosition();
    static void TestSetRainbowPalette();
    static void TestSetTexture();
    static void TestSetTime();
    static void TestSetVec2();
    static void TestSetVec3Array();
    static void TestUnbind();
    static void RunAllTests();
};
