// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file behaviortreeaction.hpp
 * @brief Action and condition leaf nodes for behavior trees.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <functional>
#include "behaviortreenode.hpp"

namespace koilo {

/**
 * @typedef ActionFunction
 * @brief Function type for actions that return NodeStatus.
 */
using ActionFunction = std::function<NodeStatus()>;

/**
 * @typedef ConditionFunction
 * @brief Function type for conditions that return bool.
 */
using ConditionFunction = std::function<bool()>;

/**
 * @class ActionNode
 * @brief Leaf node that executes a custom action function.
 */
class ActionNode : public BehaviorTreeNode {
private:
    ActionFunction action;

public:
    /**
     * @brief Constructor with action function.
     */
    ActionNode(ActionFunction action, const std::string& name = "Action");

    virtual NodeStatus Execute() override;

    KL_BEGIN_FIELDS(ActionNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ActionNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ActionNode)
        // Cannot describe function pointers
    KL_END_DESCRIBE(ActionNode)
};

/**
 * @class ConditionNode
 * @brief Leaf node that checks a condition.
 */
class ConditionNode : public BehaviorTreeNode {
private:
    ConditionFunction condition;

public:
    /**
     * @brief Constructor with condition function.
     */
    ConditionNode(ConditionFunction condition, const std::string& name = "Condition");

    virtual NodeStatus Execute() override;

    KL_BEGIN_FIELDS(ConditionNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConditionNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConditionNode)
        // Cannot describe function pointers
    KL_END_DESCRIBE(ConditionNode)
};

/**
 * @class WaitNode
 * @brief Waits for a specified duration.
 */
class WaitNode : public BehaviorTreeNode {
private:
    float duration;       ///< Wait duration (seconds)
    float elapsed;        ///< Elapsed time

public:
    /**
     * @brief Constructor with duration.
     */
    WaitNode(float duration, const std::string& name = "Wait");

    virtual NodeStatus Execute() override;
    virtual void Reset() override;

    /**
     * @brief Updates the wait timer.
     */
    void Update();

    KL_BEGIN_FIELDS(WaitNode)
        KL_FIELD(WaitNode, duration, "Duration", 0, 0),
        KL_FIELD(WaitNode, elapsed, "Elapsed", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(WaitNode)
        KL_METHOD_AUTO(WaitNode, Update, "Update")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(WaitNode)
        KL_CTOR(WaitNode, float, std::string)
    KL_END_DESCRIBE(WaitNode)
};

} // namespace koilo
