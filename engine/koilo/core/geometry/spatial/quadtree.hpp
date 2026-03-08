/**
 * @file quadtree.hpp
 * @brief Runtime quadtree for lightweight spatial partitioning.
 * @date  18/06/2025
 */
#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "../../math/vector2d.hpp"
#include "../2d/rectangle.hpp"

/**
 * @class QuadTree
 * @brief A runtime 2-D quadtree for spatial partitioning with type-erased items.
 *
 * Items are stored as opaque pointers and validated through a caller-provided
 * overlap callback. This keeps the implementation in a translation unit while
 * allowing callers to use strongly-typed wrappers at the interface level.
 */
class QuadTree {
public:
    using ItemPtr = void*;                                                ///< Opaque pointer to stored items.
    using OverlapsCallback = bool (*)(const void*, const Rectangle2D&);    ///< Callback used to test bounds overlap.

    /**
     * @class Node
     * @brief Internal quadtree node that stores items residing in a region.
     */
    class Node {
    public:
        Node(const Rectangle2D& bounds, OverlapsCallback overlaps, unsigned char depth = 0);
        ~Node();

        bool Insert(ItemPtr item);
        Node* FindLeaf(const Vector2D& point);
        void Subdivide();

        bool IsLeaf() const;
        unsigned short GetItemCount() const;
        unsigned short GetCapacity() const;
        const Rectangle2D& GetBounds() const;

        ItemPtr* GetItemsRaw();
        ItemPtr const* GetItemsRaw() const;

        template <typename T>
        T** GetItems() {
            return reinterpret_cast<T**>(GetItemsRaw());
        }

        template <typename T>
        T* const* GetItems() const {
            return reinterpret_cast<T* const*>(GetItemsRaw());
        }

    private:
        static constexpr unsigned short kMaxItems = 8; ///< Max items before subdivision.
        static constexpr unsigned char  kMaxDepth = 8; ///< Maximum tree depth.

        Rectangle2D bounds;
        unsigned short itemCount;
        std::vector<ItemPtr> items;
        std::array<std::unique_ptr<Node>, 4> children;
        OverlapsCallback overlaps;
        unsigned char depth;

        void EnsureCapacity(unsigned short newCap);
        void CreateChildren();
        unsigned short Distribute();
    };

    QuadTree(const Rectangle2D& bounds, OverlapsCallback overlaps);
    ~QuadTree();

    bool Insert(ItemPtr item);

    template <typename T>
    bool Insert(T* item) {
        return Insert(static_cast<ItemPtr>(item));
    }

    ItemPtr* QueryPointRaw(const Vector2D& point, unsigned short& countOut);

    template <typename T>
    T** QueryPoint(const Vector2D& point, unsigned short& countOut) {
        return reinterpret_cast<T**>(QueryPointRaw(point, countOut));
    }

    void Rebuild();

    Node* GetRoot();
    const Node* GetRoot() const;

    unsigned long GetItemCount() const { return totalItems; }

private:
    std::unique_ptr<Node> root;
    unsigned long totalItems;
    OverlapsCallback overlaps;
};
