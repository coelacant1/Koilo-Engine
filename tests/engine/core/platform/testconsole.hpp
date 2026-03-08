// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testconsole.hpp
 * @brief Unit tests for Console utility.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestConsole {
public:
    static void TestBegin();
    static void TestPrintString();
    static void TestPrintInt();
    static void TestPrintFloat();
    static void TestPrintlnEmpty();
    static void TestPrintlnString();
    static void TestPrintlnInt();
    static void TestPrintlnFloat();
    
    static void RunAllTests();
};

