// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file node_graph.hpp
 * @brief Visual node graph editor for shader/script/animation graphs.
 *        Built on Canvas2D for rendering, with interactive dragging and
 *        bezier connection wires.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <koilo/systems/ui/widget.hpp>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/// Port direction on a node.
enum class PortDir : uint8_t {
    Input,  ///< Port receives data
    Output  ///< Port sends data
};

/// Data type tag for port compatibility.
enum class PortType : uint8_t {
    Float, ///< Scalar float value
    Vec2,  ///< 2-component vector
    Vec3,  ///< 3-component vector
    Vec4,  ///< 4-component vector
    Color, ///< RGBA color value
    Bool,  ///< Boolean value
    Int,   ///< Integer value
    Any    ///< Wildcard, compatible with all types
};

/** @brief Return the display name string for a port type. */
inline const char* PortTypeName(PortType t) {
    switch (t) {
        case PortType::Float: return "float";
        case PortType::Vec2:  return "vec2";
        case PortType::Vec3:  return "vec3";
        case PortType::Vec4:  return "vec4";
        case PortType::Color: return "color";
        case PortType::Bool:  return "bool";
        case PortType::Int:   return "int";
        case PortType::Any:   return "any";
    }
    return "?";
}

/** @brief Return the display color for a port type. */
inline Color4 PortColor(PortType t) {
    switch (t) {
        case PortType::Float: return {120, 200, 120, 255};
        case PortType::Vec2:  return {120, 180, 220, 255};
        case PortType::Vec3:  return {180, 120, 220, 255};
        case PortType::Vec4:  return {220, 180, 120, 255};
        case PortType::Color: return {220, 220, 80, 255};
        case PortType::Bool:  return {220, 100, 100, 255};
        case PortType::Int:   return {100, 200, 200, 255};
        case PortType::Any:   return {180, 180, 180, 255};
    }
    return {180, 180, 180, 255};
}

/** @class Port @brief A port (input or output slot) on a node. */
struct Port {
    std::string name;                       ///< Display name
    PortDir     dir  = PortDir::Input;      ///< Input or output direction
    PortType    type = PortType::Float;     ///< Data type tag
    int         nodeId = -1;               ///< Owner node ID
    int         index  = 0;                ///< Port index on the node

    KL_BEGIN_FIELDS(Port)
        KL_FIELD(Port, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Port)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Port)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Port)

};

/** @class Connection @brief A connection (wire) between two ports. */
struct Connection {
    int srcNode  = -1;  ///< Source node ID
    int srcPort  = -1;  ///< Source output port index
    int dstNode  = -1;  ///< Destination node ID
    int dstPort  = -1;  ///< Destination input port index

    KL_BEGIN_FIELDS(Connection)
        KL_FIELD(Connection, srcNode, "Src node", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Connection)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Connection)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Connection)

};

/** @class GraphNode @brief A visual node in the graph. */
struct GraphNode {
    int         id = -1;                       ///< Unique node identifier
    std::string title;                         ///< Display title
    float       x = 0.0f, y = 0.0f;           ///< Position in graph space
    float       w = 140.0f, h = 0.0f;         ///< Size (h computed from port count)
    bool        selected = false;              ///< Whether this node is selected
    Color4      color{60, 60, 70, 240};        ///< Node body color
    Color4      titleColor{80, 120, 180, 255}; ///< Title bar color
    std::vector<Port> inputs;                  ///< Input ports
    std::vector<Port> outputs;                 ///< Output ports

    /** @brief Compute node height from port count. @return Computed height. */
    float ComputeHeight() {
        int portRows = static_cast<int>(std::max(inputs.size(), outputs.size()));
        h = TITLE_H + static_cast<float>(portRows) * PORT_ROW_H + 8.0f;
        return h;
    }

    static constexpr float TITLE_H = 24.0f;     ///< Title bar height in pixels
    static constexpr float PORT_ROW_H = 20.0f;  ///< Height per port row
    static constexpr float PORT_RADIUS = 5.0f;  ///< Port circle radius

    KL_BEGIN_FIELDS(GraphNode)
        KL_FIELD(GraphNode, id, "Id", -2147483648, 2147483647),
        KL_FIELD(GraphNode, h, "H", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GraphNode)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GraphNode)
        /* No reflected ctors. */
    KL_END_DESCRIBE(GraphNode)

};

/** @class NodeGraph @brief Visual node graph editor built on Canvas2D. */
class NodeGraph {
public:
    /**
     * @brief Create the canvas widget inside a parent.
     * @param ctx       UI context to create widgets in.
     * @param parentIdx Parent widget index.
     * @param id        Canvas widget string ID.
     * @param width     Canvas width in pixels.
     * @param height    Canvas height in pixels.
     * @return Widget index, or -1 on failure.
     */
    int Build(class UIContext& ctx, int parentIdx, const char* id,
              float width = 400.0f, float height = 300.0f);

    // --- Graph API ---

    /** @brief Add a node and return its ID. */
    int AddNode(const std::string& title, float x, float y) {
        int id = nextNodeId_++;
        GraphNode node;
        node.id = id;
        node.title = title;
        node.x = x;
        node.y = y;
        nodes_.push_back(std::move(node));
        return id;
    }

    /** @brief Add an input port to a node. */
    void AddInput(int nodeId, const std::string& name, PortType type = PortType::Float) {
        GraphNode* n = FindNode(nodeId);
        if (!n) return;
        Port p;
        p.name = name;
        p.dir = PortDir::Input;
        p.type = type;
        p.nodeId = nodeId;
        p.index = static_cast<int>(n->inputs.size());
        n->inputs.push_back(std::move(p));
        n->ComputeHeight();
    }

    /** @brief Add an output port to a node. */
    void AddOutput(int nodeId, const std::string& name, PortType type = PortType::Float) {
        GraphNode* n = FindNode(nodeId);
        if (!n) return;
        Port p;
        p.name = name;
        p.dir = PortDir::Output;
        p.type = type;
        p.nodeId = nodeId;
        p.index = static_cast<int>(n->outputs.size());
        n->outputs.push_back(std::move(p));
        n->ComputeHeight();
    }

    /** @brief Connect two ports. @return True on success. */
    bool Connect(int srcNode, int srcPort, int dstNode, int dstPort) {
        GraphNode* sn = FindNode(srcNode);
        GraphNode* dn = FindNode(dstNode);
        if (!sn || !dn) return false;
        if (srcPort < 0 || srcPort >= static_cast<int>(sn->outputs.size())) return false;
        if (dstPort < 0 || dstPort >= static_cast<int>(dn->inputs.size())) return false;

        // Check type compatibility
        PortType st = sn->outputs[srcPort].type;
        PortType dt = dn->inputs[dstPort].type;
        if (st != dt && st != PortType::Any && dt != PortType::Any) return false;

        // Remove existing connection to same input
        Disconnect(dstNode, dstPort);

        Connection c;
        c.srcNode = srcNode;
        c.srcPort = srcPort;
        c.dstNode = dstNode;
        c.dstPort = dstPort;
        connections_.push_back(c);
        return true;
    }

    /** @brief Disconnect an input port. */
    void Disconnect(int nodeId, int portIdx) {
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [nodeId, portIdx](const Connection& c) {
                    return c.dstNode == nodeId && c.dstPort == portIdx;
                }),
            connections_.end());
    }

    /** @brief Remove a node and all its connections. */
    void RemoveNode(int nodeId) {
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [nodeId](const Connection& c) {
                    return c.srcNode == nodeId || c.dstNode == nodeId;
                }),
            connections_.end());
        nodes_.erase(
            std::remove_if(nodes_.begin(), nodes_.end(),
                [nodeId](const GraphNode& n) { return n.id == nodeId; }),
            nodes_.end());
    }

    /** @brief Clear the current node selection. */
    void ClearSelection() {
        for (auto& n : nodes_) n.selected = false;
    }

    /** @brief Select a single node by ID, deselecting all others. */
    void SelectNode(int nodeId) {
        for (auto& n : nodes_) n.selected = (n.id == nodeId);
    }

    // --- Interaction ---

    /** @brief Handle pointer-down in canvas-local coordinates. */
    void HandlePointerDown(float cx, float cy) {
        // Check if clicking on a port
        for (auto& node : nodes_) {
            // Output ports
            for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i) {
                float px, py;
                OutputPortPos(node, i, px, py);
                if (DistSq(cx, cy, px + panX_, py + panY_) < 64.0f) {
                    draggingWire_ = true;
                    wireSrcNode_ = node.id;
                    wireSrcPort_ = i;
                    wireEndX_ = cx;
                    wireEndY_ = cy;
                    return;
                }
            }
        }

        // Check if clicking on a node body
        for (int i = static_cast<int>(nodes_.size()) - 1; i >= 0; --i) {
            auto& node = nodes_[i];
            float nx = node.x + panX_;
            float ny = node.y + panY_;
            if (cx >= nx && cx <= nx + node.w && cy >= ny && cy <= ny + node.h) {
                ClearSelection();
                node.selected = true;
                draggingNode_ = true;
                dragNodeId_ = node.id;
                dragOffsetX_ = cx - nx;
                dragOffsetY_ = cy - ny;
                // Move to front
                if (i < static_cast<int>(nodes_.size()) - 1) {
                    auto tmp = std::move(nodes_[i]);
                    nodes_.erase(nodes_.begin() + i);
                    nodes_.push_back(std::move(tmp));
                }
                return;
            }
        }

        // Click on empty space: start panning
        ClearSelection();
        panning_ = true;
        panStartX_ = cx;
        panStartY_ = cy;
        panStartOffX_ = panX_;
        panStartOffY_ = panY_;
    }

    /** @brief Handle pointer-move in canvas-local coordinates. */
    void HandlePointerMove(float cx, float cy) {
        if (draggingWire_) {
            wireEndX_ = cx;
            wireEndY_ = cy;
            return;
        }
        if (draggingNode_) {
            GraphNode* n = FindNode(dragNodeId_);
            if (n) {
                n->x = cx - dragOffsetX_ - panX_;
                n->y = cy - dragOffsetY_ - panY_;
            }
            return;
        }
        if (panning_) {
            panX_ = panStartOffX_ + (cx - panStartX_);
            panY_ = panStartOffY_ + (cy - panStartY_);
        }
    }

    /** @brief Handle pointer-up in canvas-local coordinates. */
    void HandlePointerUp(float cx, float cy) {
        if (draggingWire_) {
            // Check if dropping on an input port
            for (auto& node : nodes_) {
                for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i) {
                    float px, py;
                    InputPortPos(node, i, px, py);
                    if (DistSq(cx, cy, px + panX_, py + panY_) < 64.0f) {
                        Connect(wireSrcNode_, wireSrcPort_, node.id, i);
                        break;
                    }
                }
            }
            draggingWire_ = false;
        }
        draggingNode_ = false;
        panning_ = false;
    }

    // --- Rendering ---

    /** @brief Paint callback for Canvas2D rendering. */
    void Paint(void* canvasCtx) const;

    // --- Accessors ---
    /** @brief Get all nodes. */
    const std::vector<GraphNode>& Nodes() const { return nodes_; }
    /** @brief Get all connections. */
    const std::vector<Connection>& Connections() const { return connections_; }
    /** @brief Get the canvas widget index. */
    int CanvasIndex() const { return canvasIdx_; }
    /** @brief Get current horizontal pan offset. */
    float PanX() const { return panX_; }
    /** @brief Get current vertical pan offset. */
    float PanY() const { return panY_; }

private:
    GraphNode* FindNode(int id) {
        for (auto& n : nodes_) if (n.id == id) return &n;
        return nullptr;
    }
    const GraphNode* FindNode(int id) const {
        for (auto& n : nodes_) if (n.id == id) return &n;
        return nullptr;
    }

    void OutputPortPos(const GraphNode& n, int idx, float& px, float& py) const {
        px = n.x + n.w;
        py = n.y + GraphNode::TITLE_H + static_cast<float>(idx) * GraphNode::PORT_ROW_H + GraphNode::PORT_ROW_H * 0.5f;
    }
    void InputPortPos(const GraphNode& n, int idx, float& px, float& py) const {
        px = n.x;
        py = n.y + GraphNode::TITLE_H + static_cast<float>(idx) * GraphNode::PORT_ROW_H + GraphNode::PORT_ROW_H * 0.5f;
    }

    static float DistSq(float x0, float y0, float x1, float y1) {
        float dx = x1 - x0, dy = y1 - y0;
        return dx * dx + dy * dy;
    }

    void DrawBezier(class CanvasDrawContext& ctx,
                    float x0, float y0, float cx0, float cy0,
                    float cx1, float cy1, float x1, float y1,
                    float width, Color4 color) const;

    std::vector<GraphNode> nodes_;          ///< All nodes in the graph
    std::vector<Connection> connections_;   ///< All connections between nodes
    int nextNodeId_ = 0;                   ///< Next auto-incremented node ID
    int canvasIdx_ = -1;                   ///< Canvas widget index in UIContext

    // Pan offset
    float panX_ = 0.0f, panY_ = 0.0f;     ///< Current pan offset

    // Interaction state
    bool draggingNode_ = false;            ///< True while dragging a node
    int dragNodeId_ = -1;                  ///< ID of the node being dragged
    float dragOffsetX_ = 0.0f, dragOffsetY_ = 0.0f; ///< Grab offset within node

    bool draggingWire_ = false;            ///< True while dragging a new wire
    int wireSrcNode_ = -1;                 ///< Source node of the wire being dragged
    int wireSrcPort_ = -1;                 ///< Source port of the wire being dragged
    float wireEndX_ = 0.0f, wireEndY_ = 0.0f; ///< Current endpoint of dragged wire

    bool panning_ = false;                 ///< True while panning the canvas
    float panStartX_ = 0.0f, panStartY_ = 0.0f;     ///< Pointer position at pan start
    float panStartOffX_ = 0.0f, panStartOffY_ = 0.0f; ///< Pan offset at pan start
};

} // namespace ui
} // namespace koilo
