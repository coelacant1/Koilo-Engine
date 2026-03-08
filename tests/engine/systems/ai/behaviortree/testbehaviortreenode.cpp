// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbehaviortreenode.cpp
 * @brief Implementation of BehaviorTreeNode unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testbehaviortreenode.hpp"
#include <koilo/systems/ai/behaviortree/behaviortreenode.hpp>
#include <koilo/systems/ai/behaviortree/behaviortreeaction.hpp>

using namespace koilo;

// Mock node for testing
class MockNode : public BehaviorTreeNode {
private:
    NodeStatus returnStatus;
public:
    MockNode(NodeStatus status) : BehaviorTreeNode("Mock"), returnStatus(status) {}
    virtual NodeStatus Execute() override { return returnStatus; }
};

// === Base Node Tests ===

void TestBehaviorTreeNode::TestReset() {
    SequenceNode node;
    node.Reset();
    TEST_ASSERT_EQUAL(0, node.GetChildCount()); // Reset should work
}

// === SequenceNode Tests ===

// === SelectorNode Tests ===

// === ParallelNode Tests ===

// === InverterNode Tests ===

// === RepeaterNode Tests ===

// === SucceederNode Tests ===

void TestBehaviorTreeNode::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // BehaviorTreeNode is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBehaviorTreeNode::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBehaviorTreeNode::TestGetChildCount() {
    // TODO: Implement test for GetChildCount()
    // BehaviorTreeNode is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBehaviorTreeNode::TestGetName() {
    // TODO: Implement test for GetName()
    // BehaviorTreeNode is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBehaviorTreeNode::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBehaviorTreeNode::TestSetName() {
    // TODO: Implement test for SetName()
    // BehaviorTreeNode is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestBehaviorTreeNode::RunAllTests() {

    RUN_TEST(TestReset);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetChildCount);
    RUN_TEST(TestGetName);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetName);
}
