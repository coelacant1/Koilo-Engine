// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testgputiming.hpp
 * @brief Unit tests for GPUTimingManager.
 */

#pragma once

#include <unity.h>

class TestGPUTiming {
public:
    static void TestDefaultState();
    static void TestEnableDisable();
    static void TestBeginEndFrameWithNullDevice();
    static void TestPassTimingWithNullDevice();
    static void TestTotalTimeDefault();
    static void TestMultiplePasses();
    static void RunAllTests();
};
