#include <ptx/systems/ai/pathfinding/pathfinder.hpp>
#include <cmath>
#include <algorithm>

namespace ptx {

PathfinderGrid::PathfinderGrid(int width, int height, bool allowDiagonal)
    : width(width), height(height), allowDiagonal(allowDiagonal) {

    // Initialize grid
    nodes.reserve(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            nodes.emplace_back(x, y, true, 1.0f);
        }
    }

    // Default heuristic
    heuristic = allowDiagonal ? DiagonalDistance : ManhattanDistance;
}

PathfinderGrid::~PathfinderGrid() {
}

// === Grid Setup ===

void PathfinderGrid::SetWalkable(int x, int y, bool walkable) {
    if (GridNode* node = GetNode(x, y)) {
        node->walkable = walkable;
    }
}

void PathfinderGrid::SetCost(int x, int y, float cost) {
    if (GridNode* node = GetNode(x, y)) {
        node->cost = cost;
    }
}

GridNode* PathfinderGrid::GetNode(int x, int y) {
    if (!IsInBounds(x, y)) {
        return nullptr;
    }
    return &nodes[y * width + x];
}

const GridNode* PathfinderGrid::GetNode(int x, int y) const {
    if (!IsInBounds(x, y)) {
        return nullptr;
    }
    return &nodes[y * width + x];
}

bool PathfinderGrid::IsInBounds(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

// === Pathfinding ===

bool PathfinderGrid::FindPath(int startX, int startY, int goalX, int goalY, std::vector<GridNode>& outPath) {
    outPath.clear();

    GridNode* startNode = GetNode(startX, startY);
    GridNode* goalNode = GetNode(goalX, goalY);

    if (!startNode || !goalNode || !startNode->walkable || !goalNode->walkable) {
        return false;  // Invalid start or goal
    }

    // A* algorithm
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
    std::unordered_set<GridNode*> closedSet;
    std::unordered_map<GridNode*, AStarNode> nodeData;

    // Initialize start node
    AStarNode startAstar;
    startAstar.gridNode = startNode;
    startAstar.gCost = 0;
    startAstar.hCost = heuristic(*startNode, *goalNode);
    startAstar.fCost = startAstar.gCost + startAstar.hCost;
    startAstar.parent = nullptr;

    openSet.push(startAstar);
    nodeData[startNode] = startAstar;

    while (!openSet.empty()) {
        // Get node with lowest f cost
        AStarNode current = openSet.top();
        openSet.pop();

        // Skip if already processed
        if (closedSet.find(current.gridNode) != closedSet.end()) {
            continue;
        }

        closedSet.insert(current.gridNode);

        // Goal reached?
        if (current.gridNode == goalNode) {
            // Reconstruct path
            AStarNode* pathNode = &nodeData[goalNode];
            while (pathNode != nullptr) {
                outPath.push_back(*pathNode->gridNode);
                pathNode = pathNode->parent;
            }
            std::reverse(outPath.begin(), outPath.end());
            return true;
        }

        // Explore neighbors
        auto neighbors = GetNeighbors(current.gridNode);
        for (GridNode* neighbor : neighbors) {
            if (closedSet.find(neighbor) != closedSet.end()) {
                continue;  // Already processed
            }

            if (!neighbor->walkable) {
                continue;  // Not walkable
            }

            // Calculate costs
            float moveCost = 1.0f * neighbor->cost;

            // Diagonal movement costs more
            int dx = std::abs(neighbor->x - current.gridNode->x);
            int dy = std::abs(neighbor->y - current.gridNode->y);
            if (dx + dy == 2) {
                moveCost *= 1.414f;  // sqrt(2)
            }

            float tentativeGCost = current.gCost + moveCost;

            // Check if this path is better
            auto it = nodeData.find(neighbor);
            if (it == nodeData.end() || tentativeGCost < it->second.gCost) {
                AStarNode neighborAstar;
                neighborAstar.gridNode = neighbor;
                neighborAstar.gCost = tentativeGCost;
                neighborAstar.hCost = heuristic(*neighbor, *goalNode);
                neighborAstar.fCost = neighborAstar.gCost + neighborAstar.hCost;
                neighborAstar.parent = &nodeData[current.gridNode];

                nodeData[neighbor] = neighborAstar;
                openSet.push(neighborAstar);
            }
        }
    }

    return false;  // No path found
}

std::vector<GridNode*> PathfinderGrid::GetNeighbors(const GridNode* node) {
    std::vector<GridNode*> neighbors;

    // 4-directional
    const int dx4[] = { 0, 1, 0, -1 };
    const int dy4[] = { -1, 0, 1, 0 };

    // 8-directional (includes diagonals)
    const int dx8[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    const int dy8[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

    const int* dx = allowDiagonal ? dx8 : dx4;
    const int* dy = allowDiagonal ? dy8 : dy4;
    int count = allowDiagonal ? 8 : 4;

    for (int i = 0; i < count; ++i) {
        int nx = node->x + dx[i];
        int ny = node->y + dy[i];

        if (GridNode* neighbor = GetNode(nx, ny)) {
            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}

// === Heuristics ===

float PathfinderGrid::ManhattanDistance(const GridNode& a, const GridNode& b) {
    return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
}

float PathfinderGrid::EuclideanDistance(const GridNode& a, const GridNode& b) {
    float dx = static_cast<float>(a.x - b.x);
    float dy = static_cast<float>(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

float PathfinderGrid::DiagonalDistance(const GridNode& a, const GridNode& b) {
    float dx = static_cast<float>(std::abs(a.x - b.x));
    float dy = static_cast<float>(std::abs(a.y - b.y));
    return std::max(dx, dy);
}

} // namespace ptx
