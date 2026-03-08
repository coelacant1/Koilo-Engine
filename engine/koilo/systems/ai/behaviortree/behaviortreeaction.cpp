// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/ai/behaviortree/behaviortreeaction.hpp>
#include <koilo/core/time/timemanager.hpp>

namespace koilo {

// === ActionNode ===

koilo::ActionNode::ActionNode(ActionFunction action, const std::string& name)
    : BehaviorTreeNode(name), action(action) {
}

NodeStatus koilo::ActionNode::Execute() {
    if (action) {
        return action();
    }
    return NodeStatus::Failure;
}

// === ConditionNode ===

koilo::ConditionNode::ConditionNode(ConditionFunction condition, const std::string& name)
    : BehaviorTreeNode(name), condition(condition) {
}

NodeStatus koilo::ConditionNode::Execute() {
    if (condition) {
        return condition() ? NodeStatus::Success : NodeStatus::Failure;
    }
    return NodeStatus::Failure;
}

// === WaitNode ===

koilo::WaitNode::WaitNode(float duration, const std::string& name)
    : BehaviorTreeNode(name), duration(duration), elapsed(0.0f) {
}

NodeStatus koilo::WaitNode::Execute() {
    if (elapsed >= duration) {
        return NodeStatus::Success;
    }
    return NodeStatus::Running;
}

void koilo::WaitNode::Reset() {
    elapsed = 0.0f;
}

void koilo::WaitNode::Update() {
    float deltaTime = TimeManager::GetInstance().GetDeltaTime();
    elapsed += deltaTime;
}

} // namespace koilo
