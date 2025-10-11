#include <ptx/systems/ai/behaviortree/behaviortreeaction.hpp>

namespace ptx {

// === ActionNode ===

ActionNode::ActionNode(ActionFunction action, const std::string& name)
    : BehaviorTreeNode(name), action(action) {
}

NodeStatus ActionNode::Execute() {
    if (action) {
        return action();
    }
    return NodeStatus::Failure;
}

// === ConditionNode ===

ConditionNode::ConditionNode(ConditionFunction condition, const std::string& name)
    : BehaviorTreeNode(name), condition(condition) {
}

NodeStatus ConditionNode::Execute() {
    if (condition) {
        return condition() ? NodeStatus::Success : NodeStatus::Failure;
    }
    return NodeStatus::Failure;
}

// === WaitNode ===

WaitNode::WaitNode(float duration, const std::string& name)
    : BehaviorTreeNode(name), duration(duration), elapsed(0.0f) {
}

NodeStatus WaitNode::Execute() {
    if (elapsed >= duration) {
        return NodeStatus::Success;
    }
    return NodeStatus::Running;
}

void WaitNode::Reset() {
    elapsed = 0.0f;
}

void WaitNode::Update(float deltaTime) {
    elapsed += deltaTime;
}

} // namespace ptx
