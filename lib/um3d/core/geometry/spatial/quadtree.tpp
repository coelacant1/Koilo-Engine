#pragma once

/*----- QuadTree<T>::Node Implementation -----*/

template<typename T>
QuadTree<T>::Node::Node(const Rectangle2D& r) :
    bounds(r),
    itemCount(0),
    capacity(0),
    items(nullptr),
    children(nullptr)
{}

template<typename T>
QuadTree<T>::Node::~Node() {
    delete[] items;    // delete[] is safe on nullptr
    delete[] children; // delete[] is safe on nullptr
}

template<typename T>
void QuadTree<T>::Node::Expand(unsigned short newCap) {
    T** tmp = items;
    items = new T*[newCap];

    // Copy existing items to the new, larger array
    for (unsigned short i = 0; i < itemCount; ++i) {
        items[i] = tmp[i];
    }
    // For safety, null out the rest of the new array
    for (unsigned short i = itemCount; i < newCap; ++i) {
        items[i] = nullptr;
    }

    delete[] tmp;
    capacity = newCap;
}

template<typename T>
void QuadTree<T>::Node::CreateChildren() {
    // Prevent re-creating children
    if (children) return;

    Vector2D center = bounds.GetCenter();
    Vector2D min = bounds.GetMinimum();
    Vector2D max = bounds.GetMaximum();

    // Create four children for the four quadrants
    children = new Node[4]{
        Node(Rectangle2D(min, center)),                                       // Top-left
        Node(Rectangle2D(Vector2D(center.X, min.Y), Vector2D(max.X, center.Y))), // Top-right
        Node(Rectangle2D(Vector2D(min.X, center.Y), Vector2D(center.X, max.Y))), // Bottom-left
        Node(Rectangle2D(center, max))                                         // Bottom-right
    };
}

template<typename T>
unsigned short QuadTree<T>::Node::Distribute() {
    unsigned short movedCount = 0;
    if (!children || !itemCount) {
        return movedCount;
    }

    // This implementation uses an in-place removal algorithm.
    int i = 0;
    while (i < itemCount) {
        bool movedToChild = false;
        for (int j = 0; j < 4; ++j) {
            // If a child can accept the item, move it
            if (children[j].Insert(items[i])) {
                movedToChild = true;
                movedCount++;
                // Item was moved, so remove it from this node's list by
                // swapping with the last item and shrinking the count.
                items[i] = items[--itemCount];
                break; // Stop trying to place it in other children
            }
        }

        // If the item was not moved (e.g., it straddles the boundary),
        // leave it in this parent node and advance to the next item.
        if (!movedToChild) {
            i++;
        }
    }
    return movedCount;
}

template<typename T>
bool QuadTree<T>::Node::Insert(T* item) {
    // The item must overlap this node's bounds to be inserted.
    if (!item->Overlaps(bounds)) {
        return false;
    }

    // If this is a leaf node and it's full (and not at max depth), subdivide it.
    if (IsLeaf() && itemCount >= kMaxItems) {
        // Note: Subdivide only acts if depth is not exceeded.
        Subdivide();
    }

    // If it has children, try to insert into a child.
    if (!IsLeaf()) {
        for (int i = 0; i < 4; ++i) {
            if (children[i].Insert(item)) {
                return true; // Successfully inserted into a child
            }
        }
    }

    // If it couldn't be inserted into a child (either because this is a leaf,
    // or the item straddles the child boundaries), add it to this node's list.
    if (itemCount == capacity) {
        // Expand capacity if needed. Start with 4, then double.
        Expand(capacity > 0 ? capacity * 2 : 4);
    }
    items[itemCount++] = item;
    return true;
}

template<typename T>
void QuadTree<T>::Node::Subdivide(unsigned char depth) {
    // Do not subdivide if already at max depth or if it already has children
    if (depth >= kMaxDepth || !IsLeaf()) {
        return;
    }

    CreateChildren();

    // If child creation was successful, distribute existing items to them.
    if (children) {
        Distribute();
    }
}

template<typename T>
typename QuadTree<T>::Node* QuadTree<T>::Node::FindLeaf(const Vector2D& p) {
    if (!bounds.Contains(p)) {
        return nullptr;
    }

    // If this is a leaf, we've found our node.
    if (IsLeaf()) {
        return this;
    }

    // Otherwise, check children.
    for (unsigned char i = 0; i < 4; ++i) {
        if (Node* hit = children[i].FindLeaf(p)) {
            return hit; // Return the leaf found in a child.
        }
    }

    // Fallback: If point is in the parent but not in any child (e.g., on an internal boundary),
    // treat the parent as the containing node for query purposes.
    return this;
}


/*----- QuadTree<T> Class Implementation -----*/

template<typename T>
QuadTree<T>::QuadTree(const Rectangle2D& r)
    : root(new Node(r)), totalItems(0) {}

template<typename T>
QuadTree<T>::~QuadTree() {
    delete root;
}

template<typename T>
bool QuadTree<T>::Insert(T* item) {
    if (!root) return false;

    if (root->Insert(item)) {
        ++totalItems;
        return true;
    }
    return false;
}

template<typename T>
T** QuadTree<T>::QueryPoint(const Vector2D& p, unsigned short& countOut) {
    Node* leaf = root->FindLeaf(p);
    if (!leaf) {
        countOut = 0;
        return nullptr;
    }
    countOut = leaf->GetItemCount();
    return leaf->GetItems();
}

template<typename T>
void QuadTree<T>::Rebuild() {
    // This is a naive rebuild. It destroys and recreates the root.
    // The user must re-insert all items from an external cache.
    if (!root) return;

    Rectangle2D b = root->GetBounds();
    delete root;
    root = new Node(b);
    totalItems = 0;
}
