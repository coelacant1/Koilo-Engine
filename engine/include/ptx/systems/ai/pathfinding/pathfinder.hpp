/**
 * @file pathfinder.hpp
 * @brief A* pathfinding for grid-based navigation.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>
#include "../../../core/math/vector3d.hpp"
#include "../../../core/math/vector2d.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(GridNode)
        PTX_FIELD(GridNode, x, "X", 0, 0),
        PTX_FIELD(GridNode, y, "Y", 0, 0),
        PTX_FIELD(GridNode, walkable, "Walkable", 0, 1),
        PTX_FIELD(GridNode, cost, "Cost", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GridNode)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GridNode)
        PTX_CTOR0(GridNode),
        PTX_CTOR(GridNode, int, int, bool, float)
    PTX_END_DESCRIBE(GridNode)
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

    PTX_BEGIN_FIELDS(PathfinderGrid)
        PTX_FIELD(PathfinderGrid, width, "Width", 0, 0),
        PTX_FIELD(PathfinderGrid, height, "Height", 0, 0),
        PTX_FIELD(PathfinderGrid, allowDiagonal, "Allow diagonal", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(PathfinderGrid)
        PTX_METHOD_AUTO(PathfinderGrid, SetWalkable, "Set walkable"),
        PTX_METHOD_AUTO(PathfinderGrid, SetCost, "Set cost"),
        PTX_METHOD_AUTO(PathfinderGrid, IsInBounds, "Is in bounds"),
        PTX_METHOD_AUTO(PathfinderGrid, SetAllowDiagonal, "Set allow diagonal")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(PathfinderGrid)
        PTX_CTOR(PathfinderGrid, int, int, bool)
    PTX_END_DESCRIBE(PathfinderGrid)
};

} // namespace ptx

// Hash function for GridNode (for unordered containers)
namespace std {
    template<>
    struct hash<ptx::GridNode> {
        size_t operator()(const ptx::GridNode& node) const {
            return hash<int>()(node.x) ^ (hash<int>()(node.y) << 1);
        }
    };
}
