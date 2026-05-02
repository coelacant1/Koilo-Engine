// SPDX-License-Identifier: GPL-3.0-or-later
#include "dbvt.hpp"
#include <algorithm>
#include <vector>

namespace koilo {

float DynamicAABBTree::Area(const AABB& a) {
    const Vector3D d = a.GetSize();
    return 2.0f * (d.X * d.Y + d.Y * d.Z + d.Z * d.X);
}

AABB DynamicAABBTree::Union(const AABB& a, const AABB& b) {
    return AABB(
        Vector3D(std::min(a.min.X, b.min.X), std::min(a.min.Y, b.min.Y), std::min(a.min.Z, b.min.Z)),
        Vector3D(std::max(a.max.X, b.max.X), std::max(a.max.Y, b.max.Y), std::max(a.max.Z, b.max.Z))
    );
}

AABB DynamicAABBTree::Inflate(const AABB& a, float margin) {
    return AABB(
        Vector3D(a.min.X - margin, a.min.Y - margin, a.min.Z - margin),
        Vector3D(a.max.X + margin, a.max.Y + margin, a.max.Z + margin)
    );
}

bool DynamicAABBTree::Contains(const AABB& outer, const AABB& inner) {
    return outer.min.X <= inner.min.X && outer.min.Y <= inner.min.Y && outer.min.Z <= inner.min.Z
        && outer.max.X >= inner.max.X && outer.max.Y >= inner.max.Y && outer.max.Z >= inner.max.Z;
}

std::int32_t DynamicAABBTree::AllocateNode() {
    if (!freeList_.empty()) {
        const std::int32_t i = freeList_.back();
        freeList_.pop_back();
        nodes_[i] = Node{};
        return i;
    }
    nodes_.emplace_back();
    return static_cast<std::int32_t>(nodes_.size()) - 1;
}

void DynamicAABBTree::FreeNode(std::int32_t handle) {
    nodes_[handle].height = -1;
    nodes_[handle].parent = kNullNode;
    nodes_[handle].child1 = kNullNode;
    nodes_[handle].child2 = kNullNode;
    nodes_[handle].userData = nullptr;
    freeList_.push_back(handle);
}

std::int32_t DynamicAABBTree::Insert(const AABB& aabb, void* userData, float margin) {
    const std::int32_t leaf = AllocateNode();
    nodes_[leaf].aabb = Inflate(aabb, margin);
    nodes_[leaf].userData = userData;
    nodes_[leaf].height = 0;
    InsertLeaf(leaf);
    return leaf;
}

void DynamicAABBTree::Remove(std::int32_t handle) {
    RemoveLeaf(handle);
    FreeNode(handle);
}

bool DynamicAABBTree::Move(std::int32_t handle, const AABB& aabb, float margin) {
    if (Contains(nodes_[handle].aabb, aabb)) return false;
    RemoveLeaf(handle);
    nodes_[handle].aabb = Inflate(aabb, margin);
    InsertLeaf(handle);
    return true;
}

void DynamicAABBTree::InsertLeaf(std::int32_t leaf) {
    if (root_ == kNullNode) {
        root_ = leaf;
        nodes_[leaf].parent = kNullNode;
        return;
    }

    const AABB leafAabb = nodes_[leaf].aabb;
    std::int32_t index = root_;
    while (!nodes_[index].IsLeaf()) {
        const std::int32_t c1 = nodes_[index].child1;
        const std::int32_t c2 = nodes_[index].child2;
        const float area = Area(nodes_[index].aabb);
        const AABB combined = Union(nodes_[index].aabb, leafAabb);
        const float combinedArea = Area(combined);
        const float cost = 2.0f * combinedArea;
        const float inheritanceCost = 2.0f * (combinedArea - area);

        auto descentCost = [&](std::int32_t child) {
            const AABB merged = Union(leafAabb, nodes_[child].aabb);
            float c = Area(merged);
            if (!nodes_[child].IsLeaf()) c -= Area(nodes_[child].aabb);
            return c + inheritanceCost;
        };
        const float cost1 = descentCost(c1);
        const float cost2 = descentCost(c2);

        if (cost < cost1 && cost < cost2) break;
        index = (cost1 < cost2) ? c1 : c2;
    }
    const std::int32_t sibling = index;

    const std::int32_t oldParent = nodes_[sibling].parent;
    const std::int32_t newParent = AllocateNode();
    nodes_[newParent].parent = oldParent;
    nodes_[newParent].userData = nullptr;
    nodes_[newParent].aabb = Union(leafAabb, nodes_[sibling].aabb);
    nodes_[newParent].height = nodes_[sibling].height + 1;
    nodes_[newParent].child1 = sibling;
    nodes_[newParent].child2 = leaf;
    nodes_[sibling].parent = newParent;
    nodes_[leaf].parent = newParent;

    if (oldParent != kNullNode) {
        if (nodes_[oldParent].child1 == sibling) nodes_[oldParent].child1 = newParent;
        else nodes_[oldParent].child2 = newParent;
    } else {
        root_ = newParent;
    }

    std::int32_t i = nodes_[leaf].parent;
    while (i != kNullNode) {
        const std::int32_t c1 = nodes_[i].child1;
        const std::int32_t c2 = nodes_[i].child2;
        nodes_[i].aabb = Union(nodes_[c1].aabb, nodes_[c2].aabb);
        nodes_[i].height = 1 + std::max(nodes_[c1].height, nodes_[c2].height);
        i = nodes_[i].parent;
    }
}

void DynamicAABBTree::RemoveLeaf(std::int32_t leaf) {
    if (leaf == root_) {
        root_ = kNullNode;
        return;
    }
    const std::int32_t parent = nodes_[leaf].parent;
    const std::int32_t grand  = nodes_[parent].parent;
    const std::int32_t sibling = (nodes_[parent].child1 == leaf)
                                 ? nodes_[parent].child2 : nodes_[parent].child1;

    if (grand != kNullNode) {
        if (nodes_[grand].child1 == parent) nodes_[grand].child1 = sibling;
        else nodes_[grand].child2 = sibling;
        nodes_[sibling].parent = grand;
        FreeNode(parent);

        std::int32_t i = grand;
        while (i != kNullNode) {
            const std::int32_t c1 = nodes_[i].child1;
            const std::int32_t c2 = nodes_[i].child2;
            nodes_[i].aabb = Union(nodes_[c1].aabb, nodes_[c2].aabb);
            nodes_[i].height = 1 + std::max(nodes_[c1].height, nodes_[c2].height);
            i = nodes_[i].parent;
        }
    } else {
        root_ = sibling;
        nodes_[sibling].parent = kNullNode;
        FreeNode(parent);
    }
}

void DynamicAABBTree::Query(const AABB& aabb, const std::function<bool(std::int32_t)>& cb) const {
    if (root_ == kNullNode) return;
    std::vector<std::int32_t> stack;
    stack.reserve(64);
    stack.push_back(root_);
    while (!stack.empty()) {
        const std::int32_t i = stack.back();
        stack.pop_back();
        if (i == kNullNode) continue;
        if (!nodes_[i].aabb.Overlaps(aabb)) continue;
        if (nodes_[i].IsLeaf()) {
            if (!cb(i)) return;
        } else {
            stack.push_back(nodes_[i].child1);
            stack.push_back(nodes_[i].child2);
        }
    }
}

void DynamicAABBTree::DescendPair(std::int32_t a, std::int32_t b,
                                  std::vector<std::pair<std::int32_t, std::int32_t>>& out) const {
    if (!nodes_[a].aabb.Overlaps(nodes_[b].aabb)) return;
    const bool aLeaf = nodes_[a].IsLeaf();
    const bool bLeaf = nodes_[b].IsLeaf();
    if (aLeaf && bLeaf) {
        if (a < b) out.emplace_back(a, b);
        else       out.emplace_back(b, a);
        return;
    }
    if (bLeaf || (!aLeaf && Area(nodes_[a].aabb) > Area(nodes_[b].aabb))) {
        DescendPair(nodes_[a].child1, b, out);
        DescendPair(nodes_[a].child2, b, out);
    } else {
        DescendPair(a, nodes_[b].child1, out);
        DescendPair(a, nodes_[b].child2, out);
    }
}

void DynamicAABBTree::DescendSelf(std::int32_t a,
                                  std::vector<std::pair<std::int32_t, std::int32_t>>& out) const {
    if (nodes_[a].IsLeaf()) return;
    const std::int32_t c1 = nodes_[a].child1;
    const std::int32_t c2 = nodes_[a].child2;
    DescendSelf(c1, out);
    DescendSelf(c2, out);
    DescendPair(c1, c2, out);
}

void DynamicAABBTree::QueryAllPairs(std::vector<std::pair<std::int32_t, std::int32_t>>& out) const {
    if (root_ == kNullNode || nodes_[root_].IsLeaf()) return;
    DescendSelf(root_, out);
}

DynamicAABBTree::Quality DynamicAABBTree::ComputeQuality() const {
    Quality q;
    q.nodeCount = static_cast<std::int32_t>(NodeCount());
    if (root_ == kNullNode) return q;
    q.height = nodes_[root_].height;
    float internalArea = 0.0f;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].height < 0) continue;
        if (nodes_[i].IsLeaf()) ++q.leafCount;
        else internalArea += Area(nodes_[i].aabb);
    }
    const float rootArea = Area(nodes_[root_].aabb);
    q.areaRatio = (rootArea > 0) ? (internalArea / rootArea) : 0.0f;
    return q;
}

} // namespace koilo
