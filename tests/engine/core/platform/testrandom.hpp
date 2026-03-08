// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrandom.hpp
 * @brief Unit tests for Random utility.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestRandom {
public:
    static void TestSeed();
    static void TestIntRange();
    static void TestIntMinMax();
    static void TestIntSameSeed();
    static void TestFloatRange();
    static void TestFloatMinMax();
    static void TestFloatDistribution();
    static void TestMultipleValues();
    
    static void RunAllTests();
};

