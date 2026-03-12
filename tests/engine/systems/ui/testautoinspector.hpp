// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testautoinspector.hpp
 * @brief Unit tests for the auto-inspector generator.
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <utils/testhelpers.hpp>

class TestAutoInspector {
public:
    static void TestGenerateEmpty();
    static void TestGenerateFloat();
    static void TestGenerateIntSlider();
    static void TestGenerateBool();
    static void TestSliderBinding();
    static void TestCheckboxBinding();
    static void TestHiddenField();
    static void TestReadOnlyField();
    static void TestCategoryGrouping();
    static void TestRefreshUpdatesValues();
    static void TestNullDesc();

    static void RunAllTests();
};
