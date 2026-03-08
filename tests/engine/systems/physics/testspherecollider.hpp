// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testspherecollider.hpp
 * @brief Unit tests for SphereCollider class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestSphereCollider {
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

