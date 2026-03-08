// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbehaviortreeaction.hpp
 * @brief Unit tests for BehaviorTree action nodes.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestBehaviorTreeAction {
public:
    // ActionNode Tests
    static void TestActionNodeConstruction();
    static void TestActionNodeExecution();
    static void TestActionNodeSuccess();
    static void TestActionNodeFailure();
    static void TestActionNodeRunning();
    
    // ConditionNode Tests
    static void TestConditionNodeConstruction();
    static void TestConditionNodeTrue();
    static void TestConditionNodeFalse();
    static void TestConditionNodeCallback();
    
    // WaitNode Tests
    static void TestWaitNodeConstruction();
    static void TestWaitNodeInitialState();
    static void TestWaitNodeUpdate();
    static void TestWaitNodeComplete();
    static void TestWaitNodeReset();
    
    static void RunAllTests();
};

