// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file manifoldgenerator.hpp
 * @brief Pair-dispatched manifold generation.
 *
 * Routes a (proxyA, proxyB) pair to its narrowphase implementation:
 *   - Sphere/Sphere, Sphere/Plane, Sphere/Box: closed-form analytic.
 *   - Plane vs convex (planar against any convex): plane-pierce against support points.
 *   - All other convex/convex: GJK boolean -> EPA depth/normal/witness pair.
 *
 * The output manifold has its proxies set with `a.proxyId < b.proxyId` so the
 * downstream contact cache key is canonical. Normals point from B toward A
 * (separation direction; the impulse on A is along +normal, on B along -normal).
 *
 * Box/Box face-clipping for multi-contact stacking is a v1 follow-up (single
 * contact via EPA witness midpoint is used for now; sufficient until the
 * sequential-impulse solver starts profiling stack stability).
 */

#pragma once

#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/contactmanifold.hpp>
#include <koilo/systems/physics/bodypose.hpp>

namespace koilo {

class ManifoldGenerator {
public:
    /**
     * @brief Builds a contact manifold for the given pair.
     * @param a,b Proxy pair (any order; output is canonicalized to proxyId asc).
     * @param poseA,poseB World poses of the bodies that own a and b.
     * @param out Cleared and populated on contact. Untouched on no-contact.
     * @return true if the pair is in contact (manifold has ≥ 1 contact point).
     */
    static bool Generate(ColliderProxy* a, const BodyPose& poseA,
                         ColliderProxy* b, const BodyPose& poseB,
                         ContactManifold& out);

    /**
     * @brief Speculative-contact closest-feature query for CCD.
     *
     * Called by `PhysicsWorld` ONLY for pairs where `Generate` returned false
     * AND at least one body is flagged `IsBullet()`. Computes the
     * (signed) gap between the closest features and, if the gap is below
     * `margin`, emits a single contact with `depth = -gap` (negative).
     * The output manifold is tagged `isSpeculative = true`.
     *
     * Supported pairs (initial scope): sphere/sphere, sphere/plane,
     * sphere/box, sphere/capsule (any order). Unsupported pairs return false
     * - no speculative contact is emitted, and tunneling for those pairs is
     * a known limitation pending follow-on work.
     *
     * The `margin` is owner-side (PhysicsWorld) responsibility:
     *   `margin = positionSlop + max(0, vRelClosing) * dt`
     * where `vRelClosing` may include angular contributions via a bounding
     * radius. The generator itself does no velocity inspection - it is a
     * pure geometric closest-feature query.
     *
     * @return true if a speculative contact was emitted.
     */
    static bool GenerateSpeculative(ColliderProxy* a, const BodyPose& poseA,
                                    ColliderProxy* b, const BodyPose& poseB,
                                    float margin,
                                    ContactManifold& out);
};

} // namespace koilo
