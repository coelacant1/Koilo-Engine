// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file pathfinder.hpp
 * @brief A* pathfinding for grid-based navigation.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <functional>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct GridNode
 * @brief Represents a node in the pathfinding grid.
 */
struct GridNode {
    int x, y;               ///< Grid coordinates
    bool walkable;          ///< Can this node be walked on?
    float cost;             ///< Movement cost multiplier (1.0 = normal)

    GridNode() : x(0), y(0), walkable(true), cost(1.0f) {}
    GridNode(int x, int y, bool walkable = true, float cost = 1.0f)
        : x(x), y(y), walkable(walkable), cost(cost) {}

    bool operator==(const GridNode& other) const {
        return x == other.x && y == other.y;
    }

    KL_BEGIN_FIELDS(GridNode)
        KL_FIELD(GridNode, x, "X", 0, 0),
        KL_FIELD(GridNode, y, "Y", 0, 0),
        KL_FIELD(GridNode, walkable, "Walkable", 0, 1),
        KL_FIELD(GridNode, cost, "Cost", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(GridNode)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GridNode)
        KL_CTOR0(GridNode),
        KL_CTOR(GridNode, int, int, bool, float)
    KL_END_DESCRIBE(GridNode)
};

/**
 * @typedef HeuristicFunction
 * @brief Function type for heuristic distance calculation.
 */
using HeuristicFunction = std::function<float(const GridNode&, const GridNode&)>;

/**
 * @class PathfinderGrid
 * @brief Grid-based pathfinding with A* algorithm.
 */
class PathfinderGrid {
private:
    int width;              ///< Grid width
    int height;             ///< Grid height
    std::vector<GridNode> nodes;     ///< Grid nodes (row-major)
    bool allowDiagonal;     ///< Allow diagonal movement?
    HeuristicFunction heuristic;

    // A* internal structures
    struct AStarNode {
        GridNode* gridNode;
        float gCost;        ///< Cost from start
        float hCost;        ///< Heuristic cost to goal
        float fCost;        ///< Total cost (g + h)
        AStarNode* parent;

        AStarNode() : gridNode(nullptr), gCost(0), hCost(0), fCost(0), parent(nullptr) {}

        bool operator>(const AStarNode& other) const {
            return fCost > other.fCost;
        }

        KL_BEGIN_FIELDS(AStarNode)
            KL_FIELD(AStarNode, gridNode, "Grid node", 0, 0),
            KL_FIELD(AStarNode, gCost, "G cost", __FLT_MIN__, __FLT_MAX__)
        KL_END_FIELDS

        KL_BEGIN_METHODS(AStarNode)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(AStarNode)
            KL_CTOR0(AStarNode)
        KL_END_DESCRIBE(AStarNode)

    };

public:
    /**
     * @brief Constructor.
     */
    PathfinderGrid(int width, int height, bool allowDiagonal = false);

    /**
     * @brief Destructor.
     */
    ~PathfinderGrid();

    // === Grid Setup ===

    /**
     * @brief Sets whether a node is walkable.
     */
    void SetWalkable(int x, int y, bool walkable);

    /**
     * @brief Sets the movement cost of a node.
     */
    void SetCost(int x, int y, float cost);

    /**
     * @brief Gets a node at coordinates.
     */
    GridNode* GetNode(int x, int y);

    /**
     * @brief Gets a node at coordinates (const).
     */
    const GridNode* GetNode(int x, int y) const;

    /**
     * @brief Checks if coordinates are within grid bounds.
     */
    bool IsInBounds(int x, int y) const;

    /**
     * @brief Sets whether diagonal movement is allowed.
     */
    void SetAllowDiagonal(bool allow) { allowDiagonal = allow; }

    /**
     * @brief Sets the heuristic function.
     */
    void SetHeuristic(HeuristicFunction func) { heuristic = func; }

    // === Pathfinding ===

    /**
     * @brief Finds a path from start to goal using A*.
     * @param startX Start X coordinate.
     * @param startY Start Y coordinate.
     * @param goalX Goal X coordinate.
     * @param goalY Goal Y coordinate.
     * @param outPath Output path (list of grid coordinates).
     * @return True if path found, false otherwise.
     */
    bool FindPath(int startX, int startY, int goalX, int goalY, std::vector<GridNode>& outPath);

    /**
     * @brief Gets neighbors of a node.
     */
    std::vector<GridNode*> GetNeighbors(const GridNode* node);

    // === Heuristics ===

    /**
     * @brief Manhattan distance heuristic.
     */
    static float ManhattanDistance(const GridNode& a, const GridNode& b);

    /**
     * @brief Euclidean distance heuristic.
     */
    static float EuclideanDistance(const GridNode& a, const GridNode& b);

    /**
     * @brief Diagonal distance heuristic (Chebyshev).
     */
    static float DiagonalDistance(const GridNode& a, const GridNode& b);

    KL_BEGIN_FIELDS(PathfinderGrid)
        KL_FIELD(PathfinderGrid, width, "Width", 0, 0),
        KL_FIELD(PathfinderGrid, height, "Height", 0, 0),
        KL_FIELD(PathfinderGrid, allowDiagonal, "Allow diagonal", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PathfinderGrid)
        KL_METHOD_AUTO(PathfinderGrid, SetWalkable, "Set walkable"),
        KL_METHOD_AUTO(PathfinderGrid, SetCost, "Set cost"),
        KL_METHOD_AUTO(PathfinderGrid, IsInBounds, "Is in bounds"),
        KL_METHOD_AUTO(PathfinderGrid, SetAllowDiagonal, "Set allow diagonal")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PathfinderGrid)
        KL_CTOR(PathfinderGrid, int, int, bool)
    KL_END_DESCRIBE(PathfinderGrid)
};

} // namespace koilo
