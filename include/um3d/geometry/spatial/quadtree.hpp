/**
 * @file quadtree.hpp
 * @brief 2-D quadtree for lightweight spatial partitioning.
 * @date  18/06/2025
 * @author moepforfreedom, Coela Can't
 */
#pragma once

#include "../../math/vector2d.hpp"
#include "../2d/rectangle.hpp"
#include "../2d/triangle.hpp"

/**
 * @class QuadTree
 * @brief Hierarchical spatial index storing pointers to Triangle2D.
 *
 * Memory-friendly: raw arrays, no std::vector.
 * Depth and capacity are compile-time constants.
 */
class QuadTree {
private:
    /** @class Node
     *  @brief Internal quadtree node (implementation detail).
     */
    class Node {
    private:
        /* compile-time constants */
        static constexpr unsigned short kMaxItems = 8;
        static constexpr unsigned char  kMaxDepth = 8;

        void Expand(unsigned short newCap);
        void CreateChildren();
        unsigned short Distribute();

    public:
        /** @brief Build node covering \p r. */
        explicit Node(const Rectangle2D& r);

        /** @brief Destructor releases child nodes and entity array. */
        ~Node();

        /** @brief Insert triangle pointer if it overlaps this node. */
        bool Insert(Triangle2D* tri);

        /** @brief Recursively find leaf containing point. */
        Node* FindLeaf(const Vector2D& p);

        /** @brief Recursively subdivide based on entity count / depth. */
        void Subdivide(unsigned char depth = 0);

        /** @brief Check if node is leaf (no children). */
        bool IsLeaf() const { return children == nullptr; }

        /** @brief Axis-aligned bounds of this node. */
        Rectangle2D bounds;

        /* public for quick traversal */
        Node*     children;     ///< array[4] or nullptr
        Triangle2D** items;     ///< raw pointer array
        unsigned short itemCount;     ///< current count
        unsigned short capacity;      ///< allocated slots
    };

    Node*           root;            ///< root node
    unsigned long   totalItems;      ///< total items in tree

public:
    /** @brief Construct tree covering rectangle \p r. */
    explicit QuadTree(const Rectangle2D& r);

    /** @brief Destructor frees all nodes. */
    ~QuadTree();

    /** @brief Insert triangle pointer if it overlaps tree bounds. */
    bool Insert(Triangle2D* tri);

    /** @brief Leaf node containing point or nullptr. */
    Triangle2D** QueryPoint(const Vector2D& p, unsigned short& countOut);

    /** @brief Rebuild entire tree (clear + insert all cached items). */
    void Rebuild();

};
