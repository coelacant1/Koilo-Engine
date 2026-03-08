// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbehaviortreeaction.cpp
 * @brief Implementation of BehaviorTree action node tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testbehaviortreeaction.hpp"
#include <koilo/core/time/timemanager.hpp>
#include <koilo/systems/ai/behaviortree/behaviortreeaction.hpp>

using namespace koilo;

// === ActionNode Tests ===

void TestBehaviorTreeAction::TestActionNodeConstruction() {
    ActionNode node([]() { return NodeStatus::Success; });
    TEST_ASSERT_EQUAL_STRING("Action", node.GetName().c_str());
    
    ActionNode namedNode([]() { return NodeStatus::Success; }, "MyAction");
    TEST_ASSERT_EQUAL_STRING("MyAction", namedNode.GetName().c_str());
}

void TestBehaviorTreeAction::TestActionNodeExecution() {
    int executionCount = 0;
    
    ActionNode node([&executionCount]() {
        executionCount++;
        return NodeStatus::Success;
    });
    
    node.Execute();
    TEST_ASSERT_EQUAL(1, executionCount);
    
    node.Execute();
    TEST_ASSERT_EQUAL(2, executionCount);
}

void TestBehaviorTreeAction::TestActionNodeSuccess() {
    ActionNode node([]() { return NodeStatus::Success; });
    
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Success), static_cast<int>(result));
}

void TestBehaviorTreeAction::TestActionNodeFailure() {
    ActionNode node([]() { return NodeStatus::Failure; });
    
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Failure), static_cast<int>(result));
}

void TestBehaviorTreeAction::TestActionNodeRunning() {
    ActionNode node([]() { return NodeStatus::Running; });
    
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Running), static_cast<int>(result));
}

// === ConditionNode Tests ===

void TestBehaviorTreeAction::TestConditionNodeConstruction() {
    ConditionNode node([]() { return true; });
    TEST_ASSERT_EQUAL_STRING("Condition", node.GetName().c_str());
    
    ConditionNode namedNode([]() { return true; }, "MyCondition");
    TEST_ASSERT_EQUAL_STRING("MyCondition", namedNode.GetName().c_str());
}

void TestBehaviorTreeAction::TestConditionNodeTrue() {
    ConditionNode node([]() { return true; });
    
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Success), static_cast<int>(result));
}

void TestBehaviorTreeAction::TestConditionNodeFalse() {
    ConditionNode node([]() { return false; });
    
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Failure), static_cast<int>(result));
}

void TestBehaviorTreeAction::TestConditionNodeCallback() {
    int value = 0;
    
    ConditionNode node([&value]() { return value > 5; });
    
    NodeStatus result1 = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Failure), static_cast<int>(result1));
    
    value = 10;
    NodeStatus result2 = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Success), static_cast<int>(result2));
}

// === WaitNode Tests ===

void TestBehaviorTreeAction::TestWaitNodeConstruction() {
    WaitNode node(1.0f);
    TEST_ASSERT_EQUAL_STRING("Wait", node.GetName().c_str());
    
    WaitNode namedNode(2.0f, "MyWait");
    TEST_ASSERT_EQUAL_STRING("MyWait", namedNode.GetName().c_str());
}

void TestBehaviorTreeAction::TestWaitNodeInitialState() {
    WaitNode node(1.0f);
    
    // First execution should return Running
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Running), static_cast<int>(result));
}

void TestBehaviorTreeAction::TestWaitNodeUpdate() {
    WaitNode node(1.0f);
    
    node.Execute(); // Start waiting
    
    // Update with 0.5 seconds - should still be running
    koilo::TimeManager::GetInstance().Tick(0.5f); node.Update();
    NodeStatus result1 = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Running), static_cast<int>(result1));
    
    // Update with another 0.3 seconds - still running (total 0.8)
    koilo::TimeManager::GetInstance().Tick(0.3f); node.Update();
    NodeStatus result2 = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Running), static_cast<int>(result2));
}

void TestBehaviorTreeAction::TestWaitNodeComplete() {
    WaitNode node(1.0f);
    
    node.Execute(); // Start waiting
    
    // Update with 1.5 seconds - should complete
    koilo::TimeManager::GetInstance().Tick(1.5f); node.Update();
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Success), static_cast<int>(result));
}

void TestBehaviorTreeAction::TestWaitNodeReset() {
    WaitNode node(1.0f);
    
    node.Execute(); // Start waiting
    koilo::TimeManager::GetInstance().Tick(0.5f); node.Update();
    
    node.Reset(); // Reset the timer
    
    // Should start waiting again
    NodeStatus result = node.Execute();
    TEST_ASSERT_EQUAL(static_cast<int>(NodeStatus::Running), static_cast<int>(result));
}

void TestBehaviorTreeAction::RunAllTests() {
    RUN_TEST(TestActionNodeConstruction);
    RUN_TEST(TestActionNodeExecution);
    RUN_TEST(TestActionNodeSuccess);
    RUN_TEST(TestActionNodeFailure);
    RUN_TEST(TestActionNodeRunning);
    
    RUN_TEST(TestConditionNodeConstruction);
    RUN_TEST(TestConditionNodeTrue);
    RUN_TEST(TestConditionNodeFalse);
    RUN_TEST(TestConditionNodeCallback);
    
    RUN_TEST(TestWaitNodeInitialState);
    RUN_TEST(TestWaitNodeConstruction);
    RUN_TEST(TestWaitNodeUpdate);
    RUN_TEST(TestWaitNodeComplete);
    RUN_TEST(TestWaitNodeReset);
}
