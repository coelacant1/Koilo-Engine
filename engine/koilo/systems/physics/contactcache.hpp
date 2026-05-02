// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file contactcache.hpp
 * @brief Cross-frame contact cache for warm-starting the SI solver.
 *
 * Keyed by `(proxyA, proxyB, featureId)` with `proxyA <= proxyB` canonical
 * ordering. Backing store is `std::map` so iteration order is deterministic
 * within a process run (T0/T1 conformance). For T2 (cross-platform
 * bit-exactness) the keys must be derived from stable per-body ids rather
 * than pointers.
 *
 * Lifecycle each frame:
 *   1. `BeginFrame()` - marks every entry stale.
 *   2. Narrowphase calls `Touch(manifold)` for every active manifold; this
 *      copies prior accumulated impulses into the manifold's contacts and
 *      records the latest contact data back into the cache, marking the
 *      entry fresh.
 *   3. `EndFrame()` - drops every entry that was not touched this frame.
 */

#pragma once

#include "contact.hpp"
#include "contactmanifold.hpp"
#include "colliderproxy.hpp"
#include <cstdint>
#include <map>

namespace koilo {

struct ContactKey {
    const ColliderProxy* a;
    const ColliderProxy* b;
    std::uint64_t featureId;

    bool operator<(const ContactKey& o) const {
        if (a != o.a) return a < o.a;
        if (b != o.b) return b < o.b;
        return featureId < o.featureId;
    }
    bool operator==(const ContactKey& o) const {
        return a == o.a && b == o.b && featureId == o.featureId;
    }

    /** Build a canonically-ordered key (smaller proxy pointer first). */
    static ContactKey Make(const ColliderProxy* x, const ColliderProxy* y, std::uint64_t feat) {
        ContactKey k{};
        if (x <= y) { k.a = x; k.b = y; }
        else        { k.a = y; k.b = x; }
        k.featureId = feat;
        return k;
    }
};

class ContactCache {
public:
    struct Entry {
        Contact contact;
        bool fresh = false;
    };

    /** Marks every entry stale. Call once per simulation step. */
    void BeginFrame() {
        for (auto& kv : entries_) kv.second.fresh = false;
    }

    /**
     * @brief For each contact in the manifold:
     *  - If a cache entry exists for its featureId, copy stored accumulated
     *    impulses into the manifold's contact (warm start).
     *  - Write the manifold's current contact data back into the cache.
     *  - Mark the entry fresh.
     */
    void Touch(ContactManifold& manifold) {
        for (std::uint8_t i = 0; i < manifold.count; ++i) {
            Contact& c = manifold.contacts[i];
            const ContactKey key = ContactKey::Make(manifold.a, manifold.b, c.featureId);
            auto it = entries_.find(key);
            if (it != entries_.end()) {
                // State-aware warm-start: if the cached contact's
                // depth-sign disagrees with the current contact (penetrating
                // ↔ speculative transition), zero the carried impulses. The
                // cached impulse was computed against a different constraint
                // regime and would otherwise momentarily push the bodies in
                // the wrong direction on the first iteration after the flip.
                const bool cachedSpeculative = (it->second.contact.depth < 0.0f);
                const bool currentSpeculative = (c.depth < 0.0f);
                if (cachedSpeculative != currentSpeculative) {
                    c.accumulatedNormalImpulse = 0.0f;
                    c.accumulatedTangentImpulse[0] = 0.0f;
                    c.accumulatedTangentImpulse[1] = 0.0f;
                } else {
                    c.accumulatedNormalImpulse = it->second.contact.accumulatedNormalImpulse;
                    c.accumulatedTangentImpulse[0] = it->second.contact.accumulatedTangentImpulse[0];
                    c.accumulatedTangentImpulse[1] = it->second.contact.accumulatedTangentImpulse[1];
                }
                it->second.contact = c;
                it->second.fresh = true;
            } else {
                Entry e;
                e.contact = c;
                e.fresh = true;
                entries_.emplace(key, e);
            }
        }
    }

    /**
     * @brief Writes solver-produced impulses back into the cache for a
     * specific manifold contact. Called by the SI solver after
     * each iteration completes.
     */
    void WriteImpulses(const ColliderProxy* a, const ColliderProxy* b,
                       std::uint64_t featureId, float normalImpulse,
                       float tangentImpulse0, float tangentImpulse1) {
        const ContactKey key = ContactKey::Make(a, b, featureId);
        auto it = entries_.find(key);
        if (it == entries_.end()) return;
        it->second.contact.accumulatedNormalImpulse = normalImpulse;
        it->second.contact.accumulatedTangentImpulse[0] = tangentImpulse0;
        it->second.contact.accumulatedTangentImpulse[1] = tangentImpulse1;
    }

    /** Drops every entry that was not touched this frame. */
    void EndFrame() {
        for (auto it = entries_.begin(); it != entries_.end(); ) {
            if (!it->second.fresh) it = entries_.erase(it);
            else ++it;
        }
    }

    std::size_t Size() const { return entries_.size(); }
    void Clear() { entries_.clear(); }

    /** Lookup helper for tests / inspection. Returns nullptr if not present. */
    const Entry* Find(const ColliderProxy* a, const ColliderProxy* b,
                      std::uint64_t featureId) const {
        const ContactKey key = ContactKey::Make(a, b, featureId);
        auto it = entries_.find(key);
        return it == entries_.end() ? nullptr : &it->second;
    }

    /** Deterministic iteration via the underlying ordered map. */
    auto begin() const { return entries_.begin(); }
    auto end() const   { return entries_.end(); }

private:
    std::map<ContactKey, Entry> entries_;
};

} // namespace koilo
