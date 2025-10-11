#include <ptx/systems/ai/behaviortree/behaviortreenode.hpp>

namespace ptx {

// === BehaviorTreeNode ===

BehaviorTreeNode::BehaviorTreeNode(const std::string& name)
    : name(name) {
}

BehaviorTreeNode::~BehaviorTreeNode() {
}

void BehaviorTreeNode::Reset() {
    for (auto& child : children) {
        child->Reset();
    }
}

void BehaviorTreeNode::AddChild(std::shared_ptr<BehaviorTreeNode> child) {
    children.push_back(child);
}

// === SequenceNode ===

SequenceNode::SequenceNode(const std::string& name)
    : BehaviorTreeNode(name), currentChildIndex(0) {
}

NodeStatus SequenceNode::Execute() {
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

void SequenceNode::Reset() {
    BehaviorTreeNode::Reset();
    currentChildIndex = 0;
}

// === SelectorNode ===

SelectorNode::SelectorNode(const std::string& name)
    : BehaviorTreeNode(name), currentChildIndex(0) {
}

NodeStatus SelectorNode::Execute() {
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

void SelectorNode::Reset() {
    BehaviorTreeNode::Reset();
    currentChildIndex = 0;
}

// === ParallelNode ===

ParallelNode::ParallelNode(int successThreshold, int failureThreshold, const std::string& name)
    : BehaviorTreeNode(name), successThreshold(successThreshold), failureThreshold(failureThreshold) {
}

NodeStatus ParallelNode::Execute() {
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

InverterNode::InverterNode(const std::string& name)
    : BehaviorTreeNode(name) {
}

NodeStatus InverterNode::Execute() {
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

RepeaterNode::RepeaterNode(int repeatCount, const std::string& name)
    : BehaviorTreeNode(name), repeatCount(repeatCount), currentCount(0) {
}

NodeStatus RepeaterNode::Execute() {
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

void RepeaterNode::Reset() {
    BehaviorTreeNode::Reset();
    currentCount = 0;
}

// === SucceederNode ===

SucceederNode::SucceederNode(const std::string& name)
    : BehaviorTreeNode(name) {
}

NodeStatus SucceederNode::Execute() {
    if (children.empty()) {
        return NodeStatus::Success;
    }

    children[0]->Execute();  // Execute but ignore result
    return NodeStatus::Success;
}

} // namespace ptx
