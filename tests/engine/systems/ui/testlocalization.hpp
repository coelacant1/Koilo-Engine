// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlocalization.hpp
 * @brief Test declarations for localization string table.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/localization.hpp>

class TestLocalization {
public:
    static void TestDefaultLocale();
    static void TestSetGet();
    static void TestFallbackToKey();
    static void TestUpdateExisting();
    static void TestClear();
    static void TestRTL();
    static void TestFontScale();
    static void TestCount();
    static void TestLAlias();

    static void RunAllTests();
};
