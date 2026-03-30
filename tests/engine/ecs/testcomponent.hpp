// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcomponent.hpp
 * @brief Unit tests for Component type identification system.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestComponent {
public:
    static void TestComponentTypeIDGeneration();
    static void TestComponentTypeIDUniqueness();
    static void TestComponentTypeIDConsistency();
    static void TestComponentTypeIDCount();
    static void TestMultipleComponentTypes();

    // Component registry tests (#29)
    static void TestRegistryRegisterAndFind();
    static void TestRegistryIDsAfterCompileTime();
    static void TestRegistryDuplicateName();
    static void TestRegistryList();

    static void RunAllTests();
};

