// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file contactmanifold.hpp
 * @brief 4-point clipped contact manifold between two collider proxies.
 *
 * Manifolds are produced by the narrowphase. When more than 4 candidate
 * contacts are clipped, the manifold keeps the deepest point plus the three
 * that maximise spread (anti-degenerate selection). The current
 * implementation uses a simpler "keep the 4 deepest unique-featureId"
 * policy; full clipping may be adopted later.
 */

#pragma once

#include "contact.hpp"
#include "colliderproxy.hpp"
#include "collisionmanager.hpp"
#include <cstdint>

namespace koilo {

struct ContactManifold {
    static constexpr std::uint8_t kMaxContacts = 4;

    ColliderProxy* a = nullptr;          ///< Non-owning. May be null in tests.
    ColliderProxy* b = nullptr;
    Contact contacts[kMaxContacts];
    std::uint8_t count = 0;
    /**
     * @brief manifold was emitted by the speculative-contact pass
     * (closest-feature query for bullet pairs that did not actually overlap).
     * Speculative manifolds carry contacts with `depth <= 0` (gap stored as
     * `-depth`), are excluded from `OnCollisionEnter/Stay/Exit` callbacks, and
     * are handled with a non-positive Baumgarte bias by the solver.
     */
    bool isSpeculative = false;

    void Clear() { count = 0; isSpeculative = false; }

    /**
     * @brief Adds a contact, merging by featureId. Returns the index of the
     * inserted/updated contact, or -1 if rejected.
     */
    int AddContact(const Contact& c) {
        // Merge if the same featureId is already present (preserves accum impulses).
        for (std::uint8_t i = 0; i < count; ++i) {
            if (contacts[i].featureId == c.featureId) {
                const float prevN = contacts[i].accumulatedNormalImpulse;
                const float prevT0 = contacts[i].accumulatedTangentImpulse[0];
                const float prevT1 = contacts[i].accumulatedTangentImpulse[1];
                contacts[i] = c;
                contacts[i].accumulatedNormalImpulse = prevN;
                contacts[i].accumulatedTangentImpulse[0] = prevT0;
                contacts[i].accumulatedTangentImpulse[1] = prevT1;
                return i;
            }
        }
        if (count < kMaxContacts) {
            contacts[count] = c;
            return count++;
        }
        // Replace the shallowest contact if the incoming one is deeper.
        std::uint8_t shallow = 0;
        for (std::uint8_t i = 1; i < count; ++i) {
            if (contacts[i].depth < contacts[shallow].depth) shallow = i;
        }
        if (c.depth > contacts[shallow].depth) {
            contacts[shallow] = c;
            return shallow;
        }
        return -1;
    }

    /** Index of the deepest contact, or -1 when empty. */
    int DeepestIndex() const {
        if (count == 0) return -1;
        std::uint8_t best = 0;
        for (std::uint8_t i = 1; i < count; ++i) {
            if (contacts[i].depth > contacts[best].depth) best = i;
        }
        return best;
    }

    /**
     * @brief Builds a script-facing CollisionInfo from the deepest contact.
     * Caller passes the legacy Collider pointers since ColliderProxy does not
     * own a back-pointer to the legacy Collider type.
     */
    CollisionInfo ToCollisionInfo(Collider* colA = nullptr, Collider* colB = nullptr) const {
        CollisionInfo info;
        info.colliderA = colA;
        info.colliderB = colB;
        const int idx = DeepestIndex();
        if (idx >= 0) {
            const Contact& c = contacts[idx];
            info.contactPoint = c.point;
            info.normal = c.normal;
            info.penetrationDepth = c.depth;
        }
        return info;
    }
};

} // namespace koilo
