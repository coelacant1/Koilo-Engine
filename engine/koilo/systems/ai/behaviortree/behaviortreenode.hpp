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
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(BehaviorTreeNode)
        PTX_FIELD(BehaviorTreeNode, name, "Name", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(BehaviorTreeNode)
        PTX_METHOD_AUTO(BehaviorTreeNode, Reset, "Reset"),
        PTX_METHOD_AUTO(BehaviorTreeNode, GetName, "Get name"),
        PTX_METHOD_AUTO(BehaviorTreeNode, SetName, "Set name"),
        PTX_METHOD_AUTO(BehaviorTreeNode, GetChildCount, "Get child count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(BehaviorTreeNode)
        // Abstract class, no constructors
    PTX_END_DESCRIBE(BehaviorTreeNode)
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

    PTX_BEGIN_FIELDS(SequenceNode)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SequenceNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SequenceNode)
        PTX_CTOR0(SequenceNode),
        PTX_CTOR(SequenceNode, std::string)
    PTX_END_DESCRIBE(SequenceNode)
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

    PTX_BEGIN_FIELDS(SelectorNode)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SelectorNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SelectorNode)
        PTX_CTOR0(SelectorNode),
        PTX_CTOR(SelectorNode, std::string)
    PTX_END_DESCRIBE(SelectorNode)
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

    PTX_BEGIN_FIELDS(ParallelNode)
        PTX_FIELD(ParallelNode, successThreshold, "Success threshold", 0, 0),
        PTX_FIELD(ParallelNode, failureThreshold, "Failure threshold", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ParallelNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ParallelNode)
        PTX_CTOR0(ParallelNode),
        PTX_CTOR(ParallelNode, int, int, std::string)
    PTX_END_DESCRIBE(ParallelNode)
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

    PTX_BEGIN_FIELDS(InverterNode)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(InverterNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(InverterNode)
        PTX_CTOR0(InverterNode),
        PTX_CTOR(InverterNode, std::string)
    PTX_END_DESCRIBE(InverterNode)
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

    PTX_BEGIN_FIELDS(RepeaterNode)
        PTX_FIELD(RepeaterNode, repeatCount, "Repeat count", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(RepeaterNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(RepeaterNode)
        PTX_CTOR0(RepeaterNode),
        PTX_CTOR(RepeaterNode, int, std::string)
    PTX_END_DESCRIBE(RepeaterNode)
};

/**
 * @class SucceederNode
 * @brief Always returns Success regardless of child result.
 */
class SucceederNode : public BehaviorTreeNode {
public:
    SucceederNode(const std::string& name = "Succeeder");
    virtual NodeStatus Execute() override;

    PTX_BEGIN_FIELDS(SucceederNode)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SucceederNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SucceederNode)
        PTX_CTOR0(SucceederNode),
        PTX_CTOR(SucceederNode, std::string)
    PTX_END_DESCRIBE(SucceederNode)
};

} // namespace ptx
