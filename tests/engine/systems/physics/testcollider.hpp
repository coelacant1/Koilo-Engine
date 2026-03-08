// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollider.hpp
 * @brief Unit tests for base Collider class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestCollider {
public:

    static void TestContainsPoint();
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestGetLayer();
    static void TestGetMaterial();
    static void TestGetTag();
    static void TestGetType();
    static void TestIsEnabled();
    static void TestIsTrigger();
    static void TestParameterizedConstructor();
    static void TestRaycastHitResult();
    static void TestSetEnabled();
    static void TestSetLayer();
    static void TestSetMaterial();
    static void TestSetTag();
    static void TestSetTrigger();
    static void RunAllTests();
};

