#include "../../../include/um3d/geometry/spatial/quadtree.hpp"

/*----- Node implementation -----*/

QuadTree::Node::Node(const Rectangle2D& r) : bounds(r), children(nullptr), items(nullptr),
      itemCount(0), capacity(0) {}

QuadTree::Node::~Node() {
    if (items) delete[] items;
    if (children) delete[] children;
}

void QuadTree::Node::Expand(unsigned short newCap) {
    Triangle2D** tmp = items;
    items = new Triangle2D*[newCap];
    for (unsigned short i = 0; i < newCap; ++i) {
        items[i] = (i < itemCount) ? tmp[i] : nullptr;
    }
    if (tmp) delete[] tmp;
    capacity = newCap;
}

void QuadTree::Node::CreateChildren() {
    Vector2D cen = bounds.GetCenter();
    Vector2D min = bounds.GetMinimum();
    Vector2D max = bounds.GetMaximum();

    children = new Node[4]{
        Node(Rectangle2D(min, cen)),
        Node(Rectangle2D(Vector2D(cen.X, min.Y), Vector2D(max.X, cen.Y))),
        Node(Rectangle2D(Vector2D(min.X, cen.Y), Vector2D(cen.X, max.Y))),
        Node(Rectangle2D(cen, max))
    };
}

unsigned short QuadTree::Node::Distribute() {
    unsigned short moved = 0;
    if (!children || !itemCount) return moved;

    for (unsigned char c = 0; c < 4; ++c) {
        for (unsigned short i = 0; i < itemCount; ++i) {
            if (children[c].Insert(items[i])) { ++moved; }
        }
    }
    delete[] items;
    items = nullptr;
    itemCount = 0;
    capacity = 0;
    return moved;
}

bool QuadTree::Node::Insert(Triangle2D* tri) {
    if (!tri->Overlaps(bounds)) return false;

    if (itemCount >= kMaxItems && !children) {
        CreateChildren();
        Distribute();
    }

    if (children) {
        /* insert into first child that accepts */
        for (unsigned char i = 0; i < 4; ++i) {
            if (children[i].Insert(tri)) return true;
        }
    }

    if (itemCount == capacity) Expand(capacity ? capacity * 2 : 4);
    items[itemCount++] = tri;
    return true;
}

QuadTree::Node* QuadTree::Node::FindLeaf(const Vector2D& p) {
    if (!bounds.Contains(p)) return nullptr;
    if (!children) return this;
    for (unsigned char i = 0; i < 4; ++i) {
        Node* hit = children[i].FindLeaf(p);
        if (hit) return hit;
    }
    return nullptr;
}

void QuadTree::Node::Subdivide(unsigned char depth) {
    if (depth >= kMaxDepth) return;
    if (!children) CreateChildren();
    Distribute();
    for (unsigned char i = 0; i < 4; ++i) {
        if (children[i].itemCount > kMaxItems) {
            children[i].Subdivide(depth + 1);
        }
    }
}

/*----- QuadTree implementation -----*/

QuadTree::QuadTree(const Rectangle2D& r)
    : root(new Node(r)), totalItems(0) {}

QuadTree::~QuadTree() {
    if (root) delete root;
}

bool QuadTree::Insert(Triangle2D* tri) {
    if (!root) return false;
    bool ok = root->Insert(tri);
    if (ok) ++totalItems;
    return ok;
}

Triangle2D** QuadTree::QueryPoint(const Vector2D& p, unsigned short& countOut) {
    Node* leaf = root->FindLeaf(p);
    if (!leaf) { countOut = 0; return nullptr; }
    countOut = leaf->itemCount;
    return leaf->items;
}

void QuadTree::Rebuild() {
    // naive rebuild: recreate root, user must re-insert externally
    Rectangle2D b = root->bounds;
    delete root;
    root = new Node(b);
    totalItems = 0;
}
