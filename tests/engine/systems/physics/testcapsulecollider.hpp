// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcapsulecollider.hpp
 * @brief Unit tests for CapsuleCollider class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestCapsuleCollider {
public:
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();
    static void TestGetPosition();
    static void TestSetPosition();
    static void TestGetRadius();
    static void TestSetRadius();
    static void TestGetHeight();
    static void TestSetHeight();

    static void TestClosestPoint();
    static void TestContainsPoint();
    static void TestEdgeCases();

    static void TestScriptRaycast();
    static void RunAllTests();
};

