// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testboxcollider.hpp
 * @brief Unit tests for BoxCollider class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestBoxCollider {
public:
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();
    static void TestGetPosition();
    static void TestSetPosition();

    static void TestClosestPoint();
    static void TestContainsPoint();
    static void TestEdgeCases();

    static void TestScriptRaycast();
    static void RunAllTests();
};

