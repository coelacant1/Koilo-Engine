// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/ai/behaviortree/behaviortreenode.hpp>

namespace koilo {

// === BehaviorTreeNode ===

koilo::BehaviorTreeNode::BehaviorTreeNode(const std::string& name)
    : name(name) {
}

koilo::BehaviorTreeNode::~BehaviorTreeNode() {
}

void koilo::BehaviorTreeNode::Reset() {
    for (auto& child : children) {
        child->Reset();
    }
}

void koilo::BehaviorTreeNode::AddChild(std::shared_ptr<BehaviorTreeNode> child) {
    children.push_back(child);
}

// === SequenceNode ===

koilo::SequenceNode::SequenceNode(const std::string& name)
    : BehaviorTreeNode(name), currentChildIndex(0) {
}

NodeStatus koilo::SequenceNode::Execute() {
    // Execute children in sequence until one fails
    while (currentChildIndex < children.size()) {
        NodeStatus status = children[currentChildIndex]->Execute();

        if (status == NodeStatus::Failure) {
            currentChildIndex = 0;  // Reset for next execution
            return NodeStatus::Failure;
        }

        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }

        // Success - move to next child
        currentChildIndex++;
    }

    // All children succeeded
    currentChildIndex = 0;
    return NodeStatus::Success;
}

void koilo::SequenceNode::Reset() {
    koilo::BehaviorTreeNode::Reset();
    currentChildIndex = 0;
}

// === SelectorNode ===

koilo::SelectorNode::SelectorNode(const std::string& name)
    : BehaviorTreeNode(name), currentChildIndex(0) {
}

NodeStatus koilo::SelectorNode::Execute() {
    // Execute children in sequence until one succeeds
    while (currentChildIndex < children.size()) {
        NodeStatus status = children[currentChildIndex]->Execute();

        if (status == NodeStatus::Success) {
            currentChildIndex = 0;  // Reset for next execution
            return NodeStatus::Success;
        }

        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }

        // Failure - move to next child
        currentChildIndex++;
    }

    // All children failed
    currentChildIndex = 0;
    return NodeStatus::Failure;
}

void koilo::SelectorNode::Reset() {
    koilo::BehaviorTreeNode::Reset();
    currentChildIndex = 0;
}

// === ParallelNode ===

koilo::ParallelNode::ParallelNode(int successThreshold, int failureThreshold, const std::string& name)
    : BehaviorTreeNode(name), successThreshold(successThreshold), failureThreshold(failureThreshold) {
}

NodeStatus koilo::ParallelNode::Execute() {
    int successCount = 0;
    int failureCount = 0;

    // Execute all children
    for (auto& child : children) {
        NodeStatus status = child->Execute();

        if (status == NodeStatus::Success) {
            successCount++;
        } else if (status == NodeStatus::Failure) {
            failureCount++;
        }
    }

    // Check thresholds
    if (successCount >= successThreshold) {
        return NodeStatus::Success;
    }

    if (failureCount >= failureThreshold) {
        return NodeStatus::Failure;
    }

    return NodeStatus::Running;
}

// === InverterNode ===

koilo::InverterNode::InverterNode(const std::string& name)
    : BehaviorTreeNode(name) {
}

NodeStatus koilo::InverterNode::Execute() {
    if (children.empty()) {
        return NodeStatus::Failure;
    }

    NodeStatus status = children[0]->Execute();

    if (status == NodeStatus::Success) {
        return NodeStatus::Failure;
    } else if (status == NodeStatus::Failure) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

// === RepeaterNode ===

koilo::RepeaterNode::RepeaterNode(int repeatCount, const std::string& name)
    : BehaviorTreeNode(name), repeatCount(repeatCount), currentCount(0) {
}

NodeStatus koilo::RepeaterNode::Execute() {
    if (children.empty()) {
        return NodeStatus::Failure;
    }

    // Infinite repeat
    if (repeatCount < 0) {
        children[0]->Execute();
        return NodeStatus::Running;
    }

    // Finite repeat
    while (currentCount < repeatCount) {
        NodeStatus status = children[0]->Execute();

        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }

        currentCount++;

        if (currentCount >= repeatCount) {
            currentCount = 0;
            return NodeStatus::Success;
        }

        // Reset child for next iteration
        children[0]->Reset();
    }

    currentCount = 0;
    return NodeStatus::Success;
}

void koilo::RepeaterNode::Reset() {
    koilo::BehaviorTreeNode::Reset();
    currentCount = 0;
}

// === SucceederNode ===

koilo::SucceederNode::SucceederNode(const std::string& name)
    : BehaviorTreeNode(name) {
}

NodeStatus koilo::SucceederNode::Execute() {
    if (children.empty()) {
        return NodeStatus::Success;
    }

    children[0]->Execute();  // Execute but ignore result
    return NodeStatus::Success;
}

} // namespace koilo
