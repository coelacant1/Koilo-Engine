/**
 * @file quadtree.hpp
 * @brief A templated, 2-D quadtree for lightweight spatial partitioning.
 * @date  18/06/2025
 * @author moepforfreedom, Coela Can't
 */
#pragma once

#include <cstddef>
#include "../../math/vector2d.hpp"
#include "../2d/rectangle.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class QuadTree
 * @brief A templated, 2-D quadtree for spatial partitioning.
 *
 * This quadtree stores pointers to objects of type T.
 * The template type T must provide the following public method:
 * bool Overlaps(const Rectangle2D& bounds) const;
 *
 * This implementation is memory-conscious, using raw dynamically-sized
 * arrays instead of std::vector. The maximum depth and item capacity
 * per node are compile-time constants for performance.
 *
 * @tparam T The type of object pointers to store in the tree.
 */
template<typename T>
class QuadTree {
public:
    class Node; // Forward-declare nested class

private:
    Node* root;       ///< The root node of the quadtree.
    unsigned long   totalItems; ///< Total number of items stored in the entire tree.

public:
    /**
     * @class Node
     * @brief An internal node of the QuadTree.
     *
     * Each node represents a rectangular area and either contains items
     * directly (if it's a leaf) or is subdivided into four child nodes.
     */
    class Node {
    private:
        /* --- Compile-time constants --- */
        static constexpr unsigned short kMaxItems = 8; ///< Max items before a node subdivides.
        static constexpr unsigned char  kMaxDepth = 8; ///< Max depth of the tree.

        /* --- Node state --- */
        Rectangle2D bounds;   ///< The axis-aligned bounding box of this node.
        unsigned short itemCount; ///< Current number of items in this node.
        unsigned short capacity;  ///< Allocated size of the items array.
        T** items;     ///< Pointer to a dynamic array of item pointers.
        Node* children;  ///< Pointer to an array of 4 child nodes, or nullptr if a leaf.

        /** @brief Expands the capacity of the items array. */
        void Expand(unsigned short newCap);

        /** @brief Creates four child nodes, partitioning the current node's area. */
        void CreateChildren();

        /** @brief Distributes the node's items among its children. Returns count of moved items. */
        unsigned short Distribute();

    public:
        /** @brief Constructs a node with the given rectangular bounds. */
        explicit Node(const Rectangle2D& r);

        /** @brief Destructor that recursively deletes children and frees item storage. */
        ~Node();

        /** @brief Attempts to insert an item into this node or its children. */
        bool Insert(T* item);

        /** @brief Finds the leaf node containing a specific point. */
        Node* FindLeaf(const Vector2D& p);

        /** @brief Subdivides the node if it exceeds item capacity and is not at max depth. */
        void Subdivide(unsigned char depth = 0);

        /** @brief Checks if this node is a leaf (has no children). */
        bool IsLeaf() const { return children == nullptr; }

        /* --- Getters for node properties --- */
        unsigned short GetItemCount() const { return itemCount; }
        unsigned short GetCapacity() const { return capacity; }
        const Rectangle2D& GetBounds() const { return bounds; }
        T** GetItems() const { return items; }
        Node* GetChildren() const { return children; }
    
        PTX_BEGIN_FIELDS(Node)
            /* TODO: PTX_FIELD(Node, member, "Doc", min, max) */
        PTX_END_FIELDS
        
        PTX_BEGIN_METHODS(Node)
            PTX_METHOD_AUTO(Node, Insert, "Insert"),
            PTX_METHOD_AUTO(Node, FindLeaf, "Find leaf"),
            PTX_METHOD_AUTO(Node, Subdivide, "Subdivide"),
            PTX_METHOD_AUTO(Node, IsLeaf, "Is leaf"),
            PTX_METHOD_AUTO(Node, GetItemCount, "Get item count"),
            PTX_METHOD_AUTO(Node, GetCapacity, "Get capacity"),
            PTX_METHOD_AUTO(Node, GetBounds, "Get bounds"),
            PTX_METHOD_AUTO(Node, GetItems, "Get items"),
            PTX_METHOD_AUTO(Node, GetChildren, "Get children")
        PTX_END_METHODS
        
        PTX_BEGIN_DESCRIBE(Node)
            PTX_CTOR(Node, const Rectangle2D &)
        PTX_END_DESCRIBE(Node)
        
};

    /** @brief Constructs a QuadTree covering the specified rectangular area. */
    explicit QuadTree(const Rectangle2D& r);

    /** @brief Destroys the QuadTree, freeing all nodes and associated memory. */
    ~QuadTree();

    /** @brief Inserts an item into the tree. */
    bool Insert(T* item);

    /**
     * @brief Queries the tree to find items at a specific point.
     * @param p The point to query.
     * @param countOut [out] The number of items found.
     * @return A pointer to an array of item pointers, or nullptr if the point is outside the tree.
     */
    T** QueryPoint(const Vector2D& p, unsigned short& countOut);

    /**
     * @brief Clears the entire tree and rebuilds the root.
     * @note This version resets the tree to an empty state. The caller
     * is responsible for re-inserting all items from an external source.
     */
    void Rebuild();

    /* --- Getters for tree properties --- */
    Node* GetRoot() { return root; }
    unsigned long GetItemCount() const { return totalItems; }

    /* NOTE: QuadTree is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(QuadTree)
        /* TODO: PTX_FIELD(QuadTree, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(QuadTree)
        PTX_METHOD_AUTO(QuadTree, Insert, "Insert"),
        PTX_METHOD_AUTO(QuadTree, QueryPoint, "Query point"),
        PTX_METHOD_AUTO(QuadTree, Rebuild, "Rebuild"),
        PTX_METHOD_AUTO(QuadTree, GetRoot, "Get root"),
        PTX_METHOD_AUTO(QuadTree, GetItemCount, "Get item count")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(QuadTree)
        PTX_CTOR(QuadTree, const Rectangle2D &)
    PTX_END_DESCRIBE(QuadTree)
    
};

// Include the implementation file for the templated class.
// This is a common pattern for C++ templates.
#include "quadtree.tpp"
