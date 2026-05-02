// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testfilewatcher.hpp
 * @brief Unit tests for the FileWatcher utility.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <utils/testhelpers.hpp>

class TestFileWatcher {
public:

    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestParameterizedConstructor();
    static void TestPoll();
    static void RunAllTests();
};
