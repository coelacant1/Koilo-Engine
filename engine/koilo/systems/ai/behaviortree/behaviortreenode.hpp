// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file behaviortreenode.hpp
 * @brief Base node for behavior trees.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @enum NodeStatus
 * @brief Status returned by behavior tree nodes.
 */
enum class NodeStatus {
    Success,   ///< Node succeeded
    Failure,   ///< Node failed
    Running    ///< Node is still executing
};

/**
 * @class BehaviorTreeNode
 * @brief Abstract base class for all behavior tree nodes.
 */
class BehaviorTreeNode {
protected:
    std::string name;                                  ///< Node name (for debugging)
    std::vector<std::shared_ptr<BehaviorTreeNode>> children;  ///< Child nodes

public:
    /**
     * @brief Constructor.
     */
    BehaviorTreeNode(const std::string& name = "Node");

    /**
     * @brief Virtual destructor.
     */
    virtual ~BehaviorTreeNode();

    /**
     * @brief Executes the node logic.
     * @return Node execution status.
     */
    virtual NodeStatus Execute() = 0;

    /**
     * @brief Resets the node state.
     */
    virtual void Reset();

    /**
     * @brief Adds a child node.
     */
    void AddChild(std::shared_ptr<BehaviorTreeNode> child);

    /**
     * @brief Gets the node name.
     */
    std::string GetName() const { return name; }

    /**
     * @brief Sets the node name.
     */
    void SetName(const std::string& n) { name = n; }

    /**
     * @brief Gets child count.
     */
    size_t GetChildCount() const { return children.size(); }

    KL_BEGIN_FIELDS(BehaviorTreeNode)
        KL_FIELD(BehaviorTreeNode, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(BehaviorTreeNode)
        KL_METHOD_AUTO(BehaviorTreeNode, Reset, "Reset"),
        KL_METHOD_AUTO(BehaviorTreeNode, GetName, "Get name"),
        KL_METHOD_AUTO(BehaviorTreeNode, SetName, "Set name"),
        KL_METHOD_AUTO(BehaviorTreeNode, GetChildCount, "Get child count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BehaviorTreeNode)
        // Abstract class, no constructors
    KL_END_DESCRIBE(BehaviorTreeNode)
};

// === Composite Nodes ===

/**
 * @class SequenceNode
 * @brief Executes children in order until one fails.
 */
class SequenceNode : public BehaviorTreeNode {
private:
    size_t currentChildIndex;

public:
    SequenceNode(const std::string& name = "Sequence");
    virtual NodeStatus Execute() override;
    virtual void Reset() override;

    KL_BEGIN_FIELDS(SequenceNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SequenceNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SequenceNode)
        KL_CTOR0(SequenceNode),
        KL_CTOR(SequenceNode, std::string)
    KL_END_DESCRIBE(SequenceNode)
};

/**
 * @class SelectorNode
 * @brief Executes children in order until one succeeds.
 */
class SelectorNode : public BehaviorTreeNode {
private:
    size_t currentChildIndex;

public:
    SelectorNode(const std::string& name = "Selector");
    virtual NodeStatus Execute() override;
    virtual void Reset() override;

    KL_BEGIN_FIELDS(SelectorNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SelectorNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SelectorNode)
        KL_CTOR0(SelectorNode),
        KL_CTOR(SelectorNode, std::string)
    KL_END_DESCRIBE(SelectorNode)
};

/**
 * @class ParallelNode
 * @brief Executes all children simultaneously.
 */
class ParallelNode : public BehaviorTreeNode {
private:
    int successThreshold;  ///< Number of children that must succeed
    int failureThreshold;  ///< Number of children that must fail

public:
    ParallelNode(int successThreshold = 1, int failureThreshold = 1, const std::string& name = "Parallel");
    virtual NodeStatus Execute() override;

    KL_BEGIN_FIELDS(ParallelNode)
        KL_FIELD(ParallelNode, successThreshold, "Success threshold", 0, 0),
        KL_FIELD(ParallelNode, failureThreshold, "Failure threshold", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ParallelNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ParallelNode)
        KL_CTOR0(ParallelNode),
        KL_CTOR(ParallelNode, int, int, std::string)
    KL_END_DESCRIBE(ParallelNode)
};

// === Decorator Nodes ===

/**
 * @class InverterNode
 * @brief Inverts the result of the child (Success <-> Failure).
 */
class InverterNode : public BehaviorTreeNode {
public:
    InverterNode(const std::string& name = "Inverter");
    virtual NodeStatus Execute() override;

    KL_BEGIN_FIELDS(InverterNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(InverterNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(InverterNode)
        KL_CTOR0(InverterNode),
        KL_CTOR(InverterNode, std::string)
    KL_END_DESCRIBE(InverterNode)
};

/**
 * @class RepeaterNode
 * @brief Repeats the child N times or indefinitely.
 */
class RepeaterNode : public BehaviorTreeNode {
private:
    int repeatCount;     ///< Number of repeats (-1 = infinite)
    int currentCount;

public:
    RepeaterNode(int repeatCount = -1, const std::string& name = "Repeater");
    virtual NodeStatus Execute() override;
    virtual void Reset() override;

    KL_BEGIN_FIELDS(RepeaterNode)
        KL_FIELD(RepeaterNode, repeatCount, "Repeat count", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RepeaterNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RepeaterNode)
        KL_CTOR0(RepeaterNode),
        KL_CTOR(RepeaterNode, int, std::string)
    KL_END_DESCRIBE(RepeaterNode)
};

/**
 * @class SucceederNode
 * @brief Always returns Success regardless of child result.
 */
class SucceederNode : public BehaviorTreeNode {
public:
    SucceederNode(const std::string& name = "Succeeder");
    virtual NodeStatus Execute() override;

    KL_BEGIN_FIELDS(SucceederNode)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SucceederNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SucceederNode)
        KL_CTOR0(SucceederNode),
        KL_CTOR(SucceederNode, std::string)
    KL_END_DESCRIBE(SucceederNode)
};

} // namespace koilo
