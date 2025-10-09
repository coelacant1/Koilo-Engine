#include <ptx/core/geometry/spatial/quadtree.hpp>

#include <utility>

QuadTree::Node::Node(const Rectangle2D& boundsValue,
                     OverlapsCallback overlapsCallback,
                     unsigned char depthValue)
    : bounds(boundsValue),
      itemCount(0),
      items(),
      children{},
      overlaps(overlapsCallback),
      depth(depthValue) {}

QuadTree::Node::~Node() = default;

void QuadTree::Node::EnsureCapacity(unsigned short newCap) {
    if (items.capacity() < newCap) {
        items.reserve(newCap);
    }
}

void QuadTree::Node::CreateChildren() {
    if (!IsLeaf()) {
        return;
    }

    const Vector2D center = bounds.GetCenter();
    const Vector2D min = bounds.GetMinimum();
    const Vector2D max = bounds.GetMaximum();

    children[0] = std::make_unique<Node>(Rectangle2D(min, center), overlaps, static_cast<unsigned char>(depth + 1));
    children[1] = std::make_unique<Node>(
        Rectangle2D(Vector2D(center.X, min.Y), Vector2D(max.X, center.Y)),
        overlaps,
        static_cast<unsigned char>(depth + 1));
    children[2] = std::make_unique<Node>(
        Rectangle2D(Vector2D(min.X, center.Y), Vector2D(center.X, max.Y)),
        overlaps,
        static_cast<unsigned char>(depth + 1));
    children[3] = std::make_unique<Node>(Rectangle2D(center, max), overlaps, static_cast<unsigned char>(depth + 1));
}

unsigned short QuadTree::Node::Distribute() {
    unsigned short movedCount = 0;
    if (IsLeaf() || itemCount == 0) {
        return movedCount;
    }

    std::size_t i = 0;
    while (i < items.size()) {
        bool movedToChild = false;
        for (auto& child : children) {
            if (!child) {
                continue;
            }

            if (child->Insert(items[i])) {
                movedToChild = true;
                ++movedCount;
                items[i] = items.back();
                items.pop_back();
                break;
            }
        }

        if (!movedToChild) {
            ++i;
        }
    }

    itemCount = static_cast<unsigned short>(items.size());
    return movedCount;
}

bool QuadTree::Node::Insert(ItemPtr item) {
    if (item == nullptr || overlaps == nullptr) {
        return false;
    }

    if (!overlaps(item, bounds)) {
        return false;
    }

    if (IsLeaf() && itemCount >= kMaxItems) {
        Subdivide();
    }

    if (!IsLeaf()) {
        for (auto& child : children) {
            if (child && child->Insert(item)) {
                return true;
            }
        }
    }

    EnsureCapacity(itemCount > 0 ? static_cast<unsigned short>(itemCount * 2) : static_cast<unsigned short>(4));

    if (itemCount < items.size()) {
        items[itemCount] = item;
    } else {
        items.push_back(item);
    }

    itemCount = static_cast<unsigned short>(items.size());
    return true;
}

void QuadTree::Node::Subdivide() {
    if (depth >= kMaxDepth || !IsLeaf()) {
        return;
    }

    CreateChildren();

    if (!IsLeaf()) {
        Distribute();
    }
}

QuadTree::Node* QuadTree::Node::FindLeaf(const Vector2D& point) {
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

bool QuadTree::Node::IsLeaf() const {
    return children[0] == nullptr;
}

unsigned short QuadTree::Node::GetItemCount() const {
    return itemCount;
}

unsigned short QuadTree::Node::GetCapacity() const {
    return static_cast<unsigned short>(items.capacity());
}

const Rectangle2D& QuadTree::Node::GetBounds() const {
    return bounds;
}

QuadTree::ItemPtr* QuadTree::Node::GetItemsRaw() {
    return items.empty() ? nullptr : items.data();
}

QuadTree::ItemPtr const* QuadTree::Node::GetItemsRaw() const {
    return items.empty() ? nullptr : items.data();
}

QuadTree::QuadTree(const Rectangle2D& bounds, OverlapsCallback overlapsCallback)
    : root(std::make_unique<Node>(bounds, overlapsCallback, 0)),
      totalItems(0),
      overlaps(overlapsCallback) {}

QuadTree::~QuadTree() = default;

bool QuadTree::Insert(ItemPtr item) {
    if (!root || overlaps == nullptr || item == nullptr) {
        return false;
    }

    if (root->Insert(item)) {
        ++totalItems;
        return true;
    }

    return false;
}

QuadTree::ItemPtr* QuadTree::QueryPointRaw(const Vector2D& point, unsigned short& countOut) {
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

void QuadTree::Rebuild() {
    if (!root) {
        return;
    }

    const Rectangle2D bounds = root->GetBounds();
    root = std::make_unique<Node>(bounds, overlaps, 0);
    totalItems = 0;
}

QuadTree::Node* QuadTree::GetRoot() {
    return root.get();
}

const QuadTree::Node* QuadTree::GetRoot() const {
    return root.get();
}
