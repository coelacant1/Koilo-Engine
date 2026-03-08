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
    static void TestDefaultConstruction();
    static void TestBindUnbind();
    static void TestBindNullModule();
    static void TestDoubleBindRebinds();

    // Shading through IMaterial interface
    static void TestShadeUniformColor();
    static void TestShadeNormalShader();
    static void TestShadeUnboundReturnsPink();

    // Parameter setting
    static void TestSetFloat();
    static void TestSetVec3();

    // KSLModule direct tests
    static void TestModuleInfo();
    static void TestModuleParams();
    static void TestModuleCreateDestroy();

    static void RunAllTests();
};
