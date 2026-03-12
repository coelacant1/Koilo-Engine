// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlogbuffer.hpp
 * @brief Test declarations for log buffer.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/log_buffer.hpp>

class TestLogBuffer {
public:
    static void TestInitiallyEmpty();
    static void TestPushAndRead();
    static void TestRingBufferWrap();
    static void TestOutOfBounds();
    static void TestClear();
    static void TestCountBySeverity();
    static void TestSearchCI();
    static void TestSearchEmpty();
    static void TestMessageTruncation();
    static void TestTimestamp();

    static void RunAllTests();
};
