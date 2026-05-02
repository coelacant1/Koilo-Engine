// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file constraintgraph.hpp
 * @brief Skeleton for constraint-graph-driven soft-body solvers.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace koilo {

/**
 * @class ConstraintGraph
 * @brief Bipartite particle/constraint structure (storage only).
 */
class ConstraintGraph {
public:
    using NodeId = std::uint32_t;
    using EdgeId = std::uint32_t;

    struct Edge {
        NodeId a;
        NodeId b;
        std::uint16_t type;
        std::uint16_t flags;
    };

    NodeId AddNode(std::uint32_t externalId) {
        const NodeId n = static_cast<NodeId>(nodeIds_.size());
        nodeIds_.push_back(externalId);
        return n;
    }

    EdgeId AddEdge(NodeId a, NodeId b, std::uint16_t type) {
        const EdgeId e = static_cast<EdgeId>(edges_.size());
        edges_.push_back({a, b, type, 0});
        return e;
    }

    std::size_t NodeCount() const { return nodeIds_.size(); }
    std::size_t EdgeCount() const { return edges_.size(); }

    std::uint32_t GetNodeExternalId(NodeId n) const { return nodeIds_[n]; }
    const Edge& GetEdge(EdgeId e) const { return edges_[e]; }

    void Clear() { nodeIds_.clear(); edges_.clear(); }

private:
    std::vector<std::uint32_t> nodeIds_;
    std::vector<Edge> edges_;
};

} // namespace koilo
