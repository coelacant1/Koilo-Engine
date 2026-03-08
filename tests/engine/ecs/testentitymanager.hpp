// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testentitymanager.hpp
 * @brief Unit tests for EntityManager and Entity classes.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestEntityManager {
public:
    // Entity Tests

    // EntityManager - Basic Operations
    static void TestCreateEntity();
    static void TestDestroyEntity();

    static void TestIsEntityValid();
    
    // EntityManager - Component Operations

    // EntityManager - Queries

    // EntityManager - Edge Cases

    static void TestClear();

    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestGetEntityCount();
    static void TestParameterizedConstructor();
    static void RunAllTests();
};

