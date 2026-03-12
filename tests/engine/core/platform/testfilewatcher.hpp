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
    static void TestWatchCount();
    static void TestUnwatch();
    static void TestPollNoChange();
    static void TestPollDetectsChange();
    static void TestClear();

    static void RunAllTests();
};
