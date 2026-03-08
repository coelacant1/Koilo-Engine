// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbehaviortreenode.hpp
 * @brief Unit tests for BehaviorTreeNode and composite nodes.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestBehaviorTreeNode {
public:
    // Base Node Tests

    static void TestReset();
    
    // SequenceNode Tests

    // SelectorNode Tests

    // ParallelNode Tests

    // InverterNode Tests

    // RepeaterNode Tests

    // SucceederNode Tests

    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestGetChildCount();
    static void TestGetName();
    static void TestParameterizedConstructor();
    static void TestSetName();
    static void RunAllTests();
};

