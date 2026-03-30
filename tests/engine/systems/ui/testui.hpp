// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testui.hpp
 * @brief Unit tests for the UI wrapper class.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/ui.hpp>
#include <utils/testhelpers.hpp>

class TestUI {
public:
    static void TestDefaultConstructor();
    static void TestClear();
    static void TestViewportAccess();
    static void TestContextAccess();

    static void RunAllTests();
};
