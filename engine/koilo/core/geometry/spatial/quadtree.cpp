// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/geometry/spatial/quadtree.hpp>
#include <koilo/core/geometry/2d/shape.hpp>
#include <cstddef>


namespace koilo {

koilo::QuadTree::Node::Node(const Rectangle2D& boundsValue,
                     OverlapsCallback overlapsCallback,
                     unsigned char depthValue)
    : bounds(boundsValue),
      itemCount(0),
      items(),
      children{},
      overlaps(overlapsCallback),
      depth(depthValue) {}

koilo::QuadTree::Node::~Node() = default;

void koilo::QuadTree::Node::EnsureCapacity(unsigned short newCap) {
    if (items.capacity() < newCap) {
        items.reserve(newCap);
    }
}

void koilo::QuadTree::Node::CreateChildren() {
    if (!IsLeaf()) {
        return;
    }

    const Vector2D center = bounds.GetCenter();
    const Vector2D min = bounds.GetMinimum();
    const Vector2D max = bounds.GetMaximum();

    // Create children using Bounds constructor (min/max), not center/size constructor
    Shape::Bounds bounds0, bounds1, bounds2, bounds3;
    
    bounds0.minV = min;
    bounds0.maxV = center;
    children[0] = std::make_unique<Node>(Rectangle2D(bounds0), overlaps, static_cast<unsigned char>(depth + 1));
    
    bounds1.minV = Vector2D(center.X, min.Y);
    bounds1.maxV = Vector2D(max.X, center.Y);
    children[1] = std::make_unique<Node>(Rectangle2D(bounds1), overlaps, static_cast<unsigned char>(depth + 1));
    
    bounds2.minV = Vector2D(min.X, center.Y);
    bounds2.maxV = Vector2D(center.X, max.Y);
    children[2] = std::make_unique<Node>(Rectangle2D(bounds2), overlaps, static_cast<unsigned char>(depth + 1));
    
    bounds3.minV = center;
    bounds3.maxV = max;
    children[3] = std::make_unique<Node>(Rectangle2D(bounds3), overlaps, static_cast<unsigned char>(depth + 1));
}

unsigned short koilo::QuadTree::Node::Distribute() {
    unsigned short movedCount = 0;
    if (IsLeaf() || itemCount == 0) {
        return movedCount;
    }

    // Try inserting each item into ALL overlapping children (ProtoTracer approach)
    for (std::size_t i = 0; i < itemCount; ++i) {
        for (auto& child : children) {
            if (child && child->Insert(items[i])) {
                ++movedCount;
            }
        }
    }

    // Clear parent's items after distributing to children
    items.clear();
    itemCount = 0;
    
    return movedCount;
}

bool koilo::QuadTree::Node::Insert(ItemPtr item) {
    if (item == nullptr || overlaps == nullptr) {
        return false;
    }

    if (!overlaps(item, bounds)) {
        return false;
    }

    // If we have children, try inserting into them
    if (!IsLeaf()) {
        bool insertedIntoChild = false;
        for (auto& child : children) {
            if (child && child->Insert(item)) {
                insertedIntoChild = true;
                // Keep trying other children - item may overlap multiple
            }
        }
        // If successfully inserted into at least one child, we're done
        return insertedIntoChild;
    }

    // We're a leaf - subdivide if needed
    if (itemCount >= kMaxItems) {
        Subdivide();
        // After subdividing, try inserting into children
        if (!IsLeaf()) {
            bool insertedIntoChild = false;
            for (auto& child : children) {
                if (child && child->Insert(item)) {
                    insertedIntoChild = true;
                }
            }
            return insertedIntoChild;
        }
    }

    // Add to this leaf node
    EnsureCapacity(itemCount > 0 ? static_cast<unsigned short>(itemCount * 2)
                                 : static_cast<unsigned short>(4));

    if (itemCount < items.size()) {
        items[itemCount] = item;
    } else {
        items.push_back(item);
    }

    itemCount = static_cast<unsigned short>(items.size());
    return true;
}

void koilo::QuadTree::Node::Subdivide() {
    if (depth >= kMaxDepth || !IsLeaf()) {
        return;
    }

    CreateChildren();

    if (!IsLeaf()) {
        Distribute();
    }
}

koilo::QuadTree::Node* koilo::QuadTree::Node::FindLeaf(const Vector2D& point) {
    if (!bounds.Contains(point)) {
        return nullptr;
    }

    if (IsLeaf()) {
        return this;
    }

    for (auto& child : children) {
        if (child) {
            Node* hit = child->FindLeaf(point);
            if (hit != nullptr) {
                return hit;
            }
        }
    }

    return this;
}

bool koilo::QuadTree::Node::IsLeaf() const {
    return children[0] == nullptr;
}

unsigned short koilo::QuadTree::Node::GetItemCount() const {
    return itemCount;
}

unsigned short koilo::QuadTree::Node::GetCapacity() const {
    return static_cast<unsigned short>(items.capacity());
}

const Rectangle2D& koilo::QuadTree::Node::GetBounds() const {
    return bounds;
}

koilo::QuadTree::ItemPtr* koilo::QuadTree::Node::GetItemsRaw() {
    return items.empty() ? nullptr : items.data();
}

koilo::QuadTree::ItemPtr const* koilo::QuadTree::Node::GetItemsRaw() const {
    return items.empty() ? nullptr : items.data();
}

koilo::QuadTree::QuadTree(const Rectangle2D& bounds, OverlapsCallback overlapsCallback)
    : root(std::make_unique<Node>(bounds, overlapsCallback, 0)),
      totalItems(0),
      overlaps(overlapsCallback) {}

koilo::QuadTree::~QuadTree() = default;

bool koilo::QuadTree::Insert(ItemPtr item) {
    if (!root || overlaps == nullptr || item == nullptr) {
        return false;
    }

    if (root->Insert(item)) {
        ++totalItems;
        return true;
    }

    return false;
}

koilo::QuadTree::ItemPtr* koilo::QuadTree::QueryPointRaw(const Vector2D& point, unsigned short& countOut) {
    if (!root) {
        countOut = 0;
        return nullptr;
    }

    Node* leaf = root->FindLeaf(point);
    if (!leaf) {
        countOut = 0;
        return nullptr;
    }

    countOut = leaf->GetItemCount();
    return leaf->GetItemsRaw();
}

void koilo::QuadTree::Rebuild() {
    if (!root) {
        return;
    }

    const Rectangle2D bounds = root->GetBounds();
    root = std::make_unique<Node>(bounds, overlaps, 0);
    totalItems = 0;
}

koilo::QuadTree::Node* koilo::QuadTree::GetRoot() {
    return root.get();
}

const koilo::QuadTree::Node* koilo::QuadTree::GetRoot() const {
    return root.get();
}

} // namespace koilo
