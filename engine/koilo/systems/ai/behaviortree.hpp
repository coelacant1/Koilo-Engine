// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file behaviortree.hpp
 * @brief Behavior tree system for AI decision making.
 *
 * Node types: Sequence, Selector, Parallel, Inverter, Repeater,
 * Succeeder, Action, Condition, Wait. All fully implemented.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/systems/ai/behaviortree/behaviortreenode.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <memory>
#include <string>

namespace koilo {

/**
 * @class BehaviorTree
 * @brief Main behavior tree controller. Owns a root node and ticks each frame.
 */
class BehaviorTree {
protected:
    std::string name;
    bool running = false;
    std::shared_ptr<BehaviorTreeNode> root;

public:
    BehaviorTree() = default;
    BehaviorTree(const std::string& name) : name(name) {}

    void SetRoot(std::shared_ptr<BehaviorTreeNode> node) { root = node; }
    std::shared_ptr<BehaviorTreeNode> GetRoot() const { return root; }

    void Tick() {
        if (!running || !root) return;
        root->Execute();
    }

    void Start() { running = true; }
    void Stop() { running = false; }
    bool IsRunning() const { return running; }
    
    const std::string& GetName() const { return name; }
    void SetName(const std::string& name) { this->name = name; }

    void Reset() {
        if (root) root->Reset();
    }

    KL_BEGIN_FIELDS(BehaviorTree)
        KL_FIELD(BehaviorTree, name, "Name", 0, 0),
        KL_FIELD(BehaviorTree, running, "Running", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(BehaviorTree)
        KL_METHOD_AUTO(BehaviorTree, Start, "Start"),
        KL_METHOD_AUTO(BehaviorTree, Stop, "Stop"),
        KL_METHOD_AUTO(BehaviorTree, IsRunning, "Is running"),
        KL_METHOD_AUTO(BehaviorTree, GetName, "Get name"),
        KL_METHOD_AUTO(BehaviorTree, SetName, "Set name"),
        KL_METHOD_AUTO(BehaviorTree, Reset, "Reset tree")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BehaviorTree)
        KL_CTOR0(BehaviorTree),
        KL_CTOR(BehaviorTree, const std::string&)
    KL_END_DESCRIBE(BehaviorTree)
};

} // namespace koilo
