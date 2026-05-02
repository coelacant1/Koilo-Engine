// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file boxboxclip.hpp
 * @brief Box-box (OBB-OBB) contact via SAT + Sutherland-Hodgman face clipping.
 *
 * Produces up to 4 stable contact points for a box-box pair. The face-contact
 * path runs the 15-axis Separating Axis Test, picks the reference/incident
 * faces, and clips the incident face against the reference face's side
 * planes. The edge-contact path (when a cross-product axis wins) emits a
 * single contact at the closest points of the two edges.
 *
 * Output normal points from B toward A (matches the rest of the narrowphase).
 *
 * featureId encoding (per contact):
 *   bits  0..15 : clipped vertex index (post-clip ordering)
 *   bits 16..21 : incident face id (axis*2 + sign)
 *   bits 22..27 : reference face id (axis*2 + sign)
 *   bit  28     : refBodyIsA flag
 *   bits 29..31 : axis-test type (0 = A-face, 1 = B-face, 2 = edge-edge)
 *
 * This encoding is stable as long as the dominant separating axis and face
 * pair remain unchanged frame-to-frame, which is enough for accumulated
 * impulse warm-starting at the speeds the solver targets.
 */

#pragma once

#include <koilo/systems/physics/contactmanifold.hpp>
#include <koilo/systems/physics/shape/boxshape.hpp>
#include <koilo/systems/physics/bodypose.hpp>

namespace koilo {

/**
 * @brief Generates a multi-contact box-box manifold.
 * @return true if `out` received at least one contact. `out.a` / `out.b` are
 *         left untouched - the caller (ManifoldGenerator::Generate) is
 *         responsible for proxy assignment and canonical ordering.
 */
bool BoxBoxContact(const BoxShape& a, const BodyPose& poseA,
                   const BoxShape& b, const BodyPose& poseB,
                   ContactManifold& out);

} // namespace koilo
