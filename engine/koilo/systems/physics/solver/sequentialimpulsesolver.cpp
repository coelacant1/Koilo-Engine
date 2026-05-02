// SPDX-License-Identifier: GPL-3.0-or-later
#include "sequentialimpulsesolver.hpp"

#include "../rigidbody.hpp"
#include "../colliderproxy.hpp"
#include "../joints/joint.hpp"

#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace koilo {

namespace {

constexpr float kEffMassEps = 1e-12f;

struct BodyScratch {
    Vector3D v;
    Vector3D w;
    float    invMass;
    Matrix3x3 invI;
    bool     dynamic;   // true if invMass>0 OR invI != 0
};

struct ConstraintRow {
    int   bodyA;            // index into scratch (or -1 for "no body" / static-world)
    int   bodyB;
    Vector3D rA;
    Vector3D rB;
    Vector3D n;
    Vector3D t1;
    Vector3D t2;
    float effMassN;
    float effMassT1;
    float effMassT2;
    float velocityBias;     // Restitution + speculative gap-closing bias (velocity pass).
    float friction;
    float accN;             // accumulated normal impulse (warm-started)
    float accT1;            // tangent accumulators (NOT warm-started in 6a)
    float accT2;
    int   manifoldIdx;
    int   contactIdx;

    // 6.5b NGS: per-row local data (set on penetrating rows when positionIterations>0).
    // localA / localB are the contact point in each body's local frame (captured at
    // row-build using the start-of-step orientation). The position pass uses these
    // to recompute world contact points / r-arms / depth from current scratch poses
    // each iteration (this is what distinguishes NGS from Catto split-impulse).
    Vector3D localA;
    Vector3D localB;
    float    initialDepth;  // c.depth at row build (signed; positive = penetration)
    bool     ngsActive;     // true => participates in the NGS position pass
};

inline void ApplyImpulse(BodyScratch& body, const Vector3D& r, const Vector3D& P, bool subtract) {
    if (!body.dynamic) return;
    if (subtract) {
        body.v = body.v - P * body.invMass;
        body.w = body.w - body.invI.Multiply(r.CrossProduct(P));
    } else {
        body.v = body.v + P * body.invMass;
        body.w = body.w + body.invI.Multiply(r.CrossProduct(P));
    }
}

inline Vector3D PointVelocity(const BodyScratch& body, const Vector3D& r) {
    if (!body.dynamic) return Vector3D(0, 0, 0);
    return body.v + body.w.CrossProduct(r);
}

// Compute effective mass = 1 / ( invMa + invMb + n . ((invIa (ra x n)) x ra) + n . ((invIb (rb x n)) x rb) )
// `body` may be a non-dynamic stand-in (invMass=0, invI=zero) - those terms collapse to 0.
inline float EffectiveMass(const Vector3D& dir,
                           const BodyScratch& a, const Vector3D& rA,
                           const BodyScratch& b, const Vector3D& rB) {
    float denom = a.invMass + b.invMass;
    if (a.dynamic) {
        const Vector3D ka = a.invI.Multiply(rA.CrossProduct(dir)).CrossProduct(rA);
        denom += dir.DotProduct(ka);
    }
    if (b.dynamic) {
        const Vector3D kb = b.invI.Multiply(rB.CrossProduct(dir)).CrossProduct(rB);
        denom += dir.DotProduct(kb);
    }
    if (denom <= kEffMassEps) return 0.0f;
    return 1.0f / denom;
}

// Build a deterministic orthonormal tangent basis (t1, t2) given unit normal n.
inline void BuildTangentBasis(const Vector3D& n, Vector3D& t1, Vector3D& t2) {
    if (std::fabs(n.X) >= 0.57735f) {
        t1 = Vector3D(n.Y, -n.X, 0.0f);
    } else {
        t1 = Vector3D(0.0f, n.Z, -n.Y);
    }
    t1 = t1.UnitSphere();   // normalize
    t2 = n.CrossProduct(t1);
}

inline BodyScratch MakeStaticScratch() {
    BodyScratch s;
    s.v = Vector3D(0, 0, 0);
    s.w = Vector3D(0, 0, 0);
    s.invMass = 0.0f;
    s.invI = Matrix3x3(0, 0, 0, 0, 0, 0, 0, 0, 0);
    s.dynamic = false;
    return s;
}

} // namespace

void SequentialImpulseSolver::Solve(std::vector<ContactManifold>& manifolds,
                                    const std::vector<RigidBody*>& bodies,
                                    ContactCache& cache,
                                    float dt,
                                    const SolverConfig& cfg) {
    static const std::vector<Joint*> kNoJoints;
    Solve(manifolds, bodies, kNoJoints, cache, dt, cfg);
}

void SequentialImpulseSolver::Solve(std::vector<ContactManifold>& manifolds,
                                    const std::vector<RigidBody*>& bodies,
                                    const std::vector<Joint*>& joints,
                                    ContactCache& cache,
                                    float dt,
                                    const SolverConfig& cfg) {
    if (dt <= 0.0f) return;
    if (manifolds.empty() && joints.empty()) return;

    // Per-body scratch state, indexed by bodyId. We allocate one extra slot
    // (index = bodies.size()) as a "static world" stand-in so rows can index
    // a body unconditionally.
    std::vector<BodyScratch> scratch;
    scratch.reserve(bodies.size() + 1);
    for (RigidBody* rb : bodies) {
        BodyScratch s;
        if (rb && !rb->IsStatic() && !rb->IsSleeping()) {
            s.v = rb->GetVelocity();
            s.w = rb->GetAngularVelocity();
            s.invMass = rb->GetInverseMass();
            s.invI = rb->GetInverseInertiaWorld();
            s.dynamic = (s.invMass > 0.0f);
        } else {
            s = MakeStaticScratch();
        }
        scratch.push_back(s);
    }
    scratch.push_back(MakeStaticScratch());
    const int kStaticIdx = static_cast<int>(scratch.size()) - 1;

    auto bodyIndex = [&](const ColliderProxy* p) -> int {
        if (!p) return kStaticIdx;
        if (p->bodyId == ColliderProxy::kInvalidHandle) return kStaticIdx;
        if (p->bodyId >= bodies.size()) return kStaticIdx;
        return static_cast<int>(p->bodyId);
    };

    // Build flat constraint-row list.
    std::vector<ConstraintRow> rows;
    rows.reserve(manifolds.size() * 4);

    for (std::size_t mi = 0; mi < manifolds.size(); ++mi) {
        ContactManifold& m = manifolds[mi];
        const int ia = bodyIndex(m.a);
        const int ib = bodyIndex(m.b);
        if (ia == ib) continue;                       // self-contact guard
        if (!scratch[ia].dynamic && !scratch[ib].dynamic) continue; // static vs static

        const Vector3D posA = (m.a && bodies[m.a->bodyId]) ? bodies[m.a->bodyId]->GetPose().position : Vector3D(0, 0, 0);
        const Vector3D posB = (m.b && bodies[m.b->bodyId]) ? bodies[m.b->bodyId]->GetPose().position : Vector3D(0, 0, 0);
        const Quaternion orientA0 = (m.a && bodies[m.a->bodyId]) ? bodies[m.a->bodyId]->GetPose().orientation : Quaternion();
        const Quaternion orientB0 = (m.b && bodies[m.b->bodyId]) ? bodies[m.b->bodyId]->GetPose().orientation : Quaternion();

        const float fricA = (m.a && bodies[m.a->bodyId]) ? bodies[m.a->bodyId]->GetFriction() : 0.0f;
        const float fricB = (m.b && bodies[m.b->bodyId]) ? bodies[m.b->bodyId]->GetFriction() : 0.0f;
        const float restA = (m.a && bodies[m.a->bodyId]) ? bodies[m.a->bodyId]->GetRestitution() : 0.0f;
        const float restB = (m.b && bodies[m.b->bodyId]) ? bodies[m.b->bodyId]->GetRestitution() : 0.0f;
        const float friction    = std::sqrt(std::max(0.0f, fricA) * std::max(0.0f, fricB));
        const float restitution = std::max(restA, restB);

        for (std::uint8_t ci = 0; ci < m.count; ++ci) {
            Contact& c = m.contacts[ci];
            ConstraintRow row;
            row.bodyA = ia;
            row.bodyB = ib;
            row.rA = c.point - posA;
            row.rB = c.point - posB;
            row.n  = c.normal;
            BuildTangentBasis(row.n, row.t1, row.t2);

            row.effMassN  = EffectiveMass(row.n,  scratch[ia], row.rA, scratch[ib], row.rB);
            row.effMassT1 = EffectiveMass(row.t1, scratch[ia], row.rA, scratch[ib], row.rB);
            row.effMassT2 = EffectiveMass(row.t2, scratch[ia], row.rA, scratch[ib], row.rB);

            // Initial relative normal velocity for restitution gating.
            const Vector3D vRel = PointVelocity(scratch[ia], row.rA) - PointVelocity(scratch[ib], row.rB);
            const float vn0 = vRel.DotProduct(row.n);

            // Bias formulation depends on whether this is a penetrating or
            // speculative (gap) contact. Speculative contacts have c.depth<0
            // (negative depth = positive separation gap = -c.depth).
            //
            // 6.5b NGS: when cfg.positionIterations>0, the penetration
            // Baumgarte bias is moved out of the velocity pass into a
            // separate position pass that mutates a scratch pose with
            // per-iteration depth/Jacobian re-evaluation. The velocity
            // pass then carries only restitution + speculative gap-closing.
            float velocityBias = 0.0f;
            bool ngsActive = false;
            const bool useNgs = (cfg.positionIterations > 0);
            if (c.depth < 0.0f) {
                // Speculative: target post-impulse vn = c.depth/dt (negative,
                // i.e. allow closing at exactly the gap-closing rate). The
                // accN >= 0 clamp ensures we never invent separation force, so
                // the row only kicks in when the actual closing velocity
                // exceeds gap/dt - that's the physical anti-tunneling limit
                // and we must NOT cap it via maxLinearCorrection (which is a
                // stability cap for spurious deep penetrations on contacting
                // bodies, not for fast pre-contact bullets).
                velocityBias = c.depth / dt;
                // No restitution on speculative contacts and no NGS row.
            } else {
                // Penetrating: Baumgarte position-error term + optional restitution.
                const float penetration = std::max(0.0f, c.depth - cfg.positionSlop);
                float positionTerm = (cfg.baumgarteBeta / dt) * penetration;
                const float vMax = cfg.maxLinearCorrection / dt;
                if (positionTerm > vMax) positionTerm = vMax;

                float restitutionBias = 0.0f;
                if (vn0 < -cfg.restitutionSlop) {
                    restitutionBias = -restitution * vn0;     // positive (target separation speed)
                }

                if (useNgs) {
                    // Velocity pass keeps only restitution; NGS pass handles penetration.
                    velocityBias = restitutionBias;
                    ngsActive = (penetration > 0.0f);
                } else {
                    // Pre-6.5b behavior: Baumgarte folded into velocity bias.
                    velocityBias = positionTerm + restitutionBias;
                }
            }
            row.velocityBias = velocityBias;
            row.friction = friction;
            row.accN  = c.accumulatedNormalImpulse * cfg.warmStartScale;
            row.accT1 = 0.0f;   // tangent warm-start disabled (basis instability)
            row.accT2 = 0.0f;
            row.manifoldIdx = static_cast<int>(mi);
            row.contactIdx  = static_cast<int>(ci);

            // NGS per-row data: capture contact point in each body's local
            // frame so the position pass can recompute world contact / r-arm
            // / depth from updated scratch poses each iteration.
            if (ngsActive) {
                row.localA = orientA0.UnrotateVector(row.rA);
                row.localB = orientB0.UnrotateVector(row.rB);
                row.initialDepth = c.depth;
                row.ngsActive = true;
            } else {
                row.localA = Vector3D(0, 0, 0);
                row.localB = Vector3D(0, 0, 0);
                row.initialDepth = 0.0f;
                row.ngsActive = false;
            }

            rows.push_back(row);
        }
    }

    // ---- Build joint constraint rows. ----
    // Joints don't have ColliderProxy, so we need a separate body-pointer to
    // scratch-index map. Built lazily - only if joints are non-empty.
    struct JointRowSlot {
        JointRow row;
        int      bodyA;
        int      bodyB;
        int      jointIdx;
        int      rowOffset;     // index within the joint's row array (for writeback ordering)
    };
    std::vector<JointRowSlot> jointRows;
    std::vector<int>          jointRowCounts;     // count per joint, for writeback
    std::vector<int>          jointRowStart;      // starting index in jointRows per joint
    std::unordered_map<const RigidBody*, int> bodyPtrToScratch;

    if (!joints.empty()) {
        bodyPtrToScratch.reserve(bodies.size());
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i]) bodyPtrToScratch.emplace(bodies[i], static_cast<int>(i));
        }
        auto resolveBody = [&](RigidBody* rb) -> int {
            if (!rb) return kStaticIdx;
            auto it = bodyPtrToScratch.find(rb);
            return (it != bodyPtrToScratch.end()) ? it->second : kStaticIdx;
        };

        jointRowCounts.resize(joints.size(), 0);
        jointRowStart.resize(joints.size(), 0);
        jointRows.reserve(joints.size() * 3);

        JointBuildContext ctx;
        ctx.dt = dt;
        ctx.baumgarteBeta = cfg.jointBaumgarteBeta;
        JointRow tmpRows[Joint::kMaxJointRows];

        for (std::size_t ji = 0; ji < joints.size(); ++ji) {
            Joint* j = joints[ji];
            if (!j) { jointRowStart[ji] = static_cast<int>(jointRows.size()); continue; }

            const int ia = resolveBody(j->GetBodyA());
            const int ib = resolveBody(j->GetBodyB());
            if (ia == ib) { jointRowStart[ji] = static_cast<int>(jointRows.size()); continue; }
            if (!scratch[ia].dynamic && !scratch[ib].dynamic) {
                // Both anchors immovable - joint can't do anything.
                jointRowStart[ji] = static_cast<int>(jointRows.size());
                continue;
            }

            // Fill ctx from scratch (post-contact-warmstart velocities are
            // not yet applied at this point - joints see fresh body state).
            ctx.poseA = j->GetBodyA() ? j->GetBodyA()->GetPose() : BodyPose();
            ctx.poseB = j->GetBodyB() ? j->GetBodyB()->GetPose() : BodyPose();
            ctx.linVelA = scratch[ia].v;
            ctx.angVelA = scratch[ia].w;
            ctx.linVelB = scratch[ib].v;
            ctx.angVelB = scratch[ib].w;
            ctx.invMassA = scratch[ia].invMass;
            ctx.invMassB = scratch[ib].invMass;
            ctx.invInertiaA = scratch[ia].invI;
            ctx.invInertiaB = scratch[ib].invI;
            ctx.dynamicA = scratch[ia].dynamic;
            ctx.dynamicB = scratch[ib].dynamic;

            const int n = j->BuildRows(ctx, tmpRows);
            jointRowStart[ji]  = static_cast<int>(jointRows.size());
            jointRowCounts[ji] = n;
            for (int r = 0; r < n; ++r) {
                JointRowSlot s;
                s.row       = tmpRows[r];
                s.bodyA     = ia;
                s.bodyB     = ib;
                s.jointIdx  = static_cast<int>(ji);
                s.rowOffset = r;
                jointRows.push_back(s);
            }
        }
    }

    if (rows.empty() && jointRows.empty()) return;

    // ---- Warm-start: apply cached normal impulse once. ----
    for (ConstraintRow& row : rows) {
        if (row.accN == 0.0f) continue;
        const Vector3D P = row.n * row.accN;
        ApplyImpulse(scratch[row.bodyA], row.rA, P, /*subtract=*/false);
        ApplyImpulse(scratch[row.bodyB], row.rB, P, /*subtract=*/true);
    }

    // ---- Joint warm-start: apply cached impulse along each row's full
    // 6-DOF Jacobian. P_lin = lin*acc, P_ang = ang*acc - applied directly
    // to v and w so we don't lose the angular component (unlike contacts
    // which use ApplyImpulse with a torque arm).
    for (JointRowSlot& s : jointRows) {
        if (s.row.accImpulse == 0.0f) continue;
        const float a = s.row.accImpulse;
        BodyScratch& A = scratch[s.bodyA];
        BodyScratch& B = scratch[s.bodyB];
        if (A.dynamic) {
            A.v = A.v + s.row.linA * (a * A.invMass);
            A.w = A.w + A.invI.Multiply(s.row.angA * a);
        }
        if (B.dynamic) {
            B.v = B.v + s.row.linB * (a * B.invMass);
            B.w = B.w + B.invI.Multiply(s.row.angB * a);
        }
    }

    // ---- Velocity iterations. ----
    for (int iter = 0; iter < cfg.velocityIterations; ++iter) {
        for (ConstraintRow& row : rows) {
            BodyScratch& A = scratch[row.bodyA];
            BodyScratch& B = scratch[row.bodyB];

            // Normal impulse.
            const Vector3D vRel = PointVelocity(A, row.rA) - PointVelocity(B, row.rB);
            const float vn = vRel.DotProduct(row.n);
            float lambda = row.effMassN * (-vn + row.velocityBias);
            const float newAccN = std::max(0.0f, row.accN + lambda);
            lambda = newAccN - row.accN;
            row.accN = newAccN;
            if (lambda != 0.0f) {
                const Vector3D P = row.n * lambda;
                ApplyImpulse(A, row.rA, P, false);
                ApplyImpulse(B, row.rB, P, true);
            }

            // Friction cap derived from the *current* accumulated normal impulse.
            const float maxF = row.friction * row.accN;

            // Tangent 1.
            {
                const Vector3D vRelT = PointVelocity(A, row.rA) - PointVelocity(B, row.rB);
                const float vt = vRelT.DotProduct(row.t1);
                float lt = row.effMassT1 * (-vt);
                const float newAcc = std::clamp(row.accT1 + lt, -maxF, maxF);
                lt = newAcc - row.accT1;
                row.accT1 = newAcc;
                if (lt != 0.0f) {
                    const Vector3D P = row.t1 * lt;
                    ApplyImpulse(A, row.rA, P, false);
                    ApplyImpulse(B, row.rB, P, true);
                }
            }

            // Tangent 2.
            {
                const Vector3D vRelT = PointVelocity(A, row.rA) - PointVelocity(B, row.rB);
                const float vt = vRelT.DotProduct(row.t2);
                float lt = row.effMassT2 * (-vt);
                const float newAcc = std::clamp(row.accT2 + lt, -maxF, maxF);
                lt = newAcc - row.accT2;
                row.accT2 = newAcc;
                if (lt != 0.0f) {
                    const Vector3D P = row.t2 * lt;
                    ApplyImpulse(A, row.rA, P, false);
                    ApplyImpulse(B, row.rB, P, true);
                }
            }
        }

        // ---- Joint rows. Iterate after contacts each iteration. ----
        for (JointRowSlot& s : jointRows) {
            BodyScratch& A = scratch[s.bodyA];
            BodyScratch& B = scratch[s.bodyB];
            // Jv = linA . vA + angA . wA + linB . vB + angB . wB.
            float Jv = 0.0f;
            if (A.dynamic) Jv += s.row.linA.DotProduct(A.v) + s.row.angA.DotProduct(A.w);
            if (B.dynamic) Jv += s.row.linB.DotProduct(B.v) + s.row.angB.DotProduct(B.w);
            float lambda = s.row.effMass * (-Jv + s.row.bias);
            const float oldAcc = s.row.accImpulse;
            const float newAcc = std::clamp(oldAcc + lambda, s.row.lowerImpulse, s.row.upperImpulse);
            lambda = newAcc - oldAcc;
            s.row.accImpulse = newAcc;
            if (lambda != 0.0f) {
                if (A.dynamic) {
                    A.v = A.v + s.row.linA * (lambda * A.invMass);
                    A.w = A.w + A.invI.Multiply(s.row.angA * lambda);
                }
                if (B.dynamic) {
                    B.v = B.v + s.row.linB * (lambda * B.invMass);
                    B.w = B.w + B.invI.Multiply(s.row.angB * lambda);
                }
            }
        }
    }

    // ---- Position pass. ----
    // Mutates a scratch pose per body (separate from real velocity -> no KE
    // injection). Each iteration re-derives world contact points / r-arms /
    // depth from the CURRENT scratch poses; this re-linearization is what
    // distinguishes NGS from Catto split-impulse and is what allows it to
    // converge for stacking/chain configurations where simple split-impulse
    // oscillates with the iteration count.
    //
    // Frozen across iterations (all initialised once from start-of-step state):
    //   - row.n, row.t1, row.t2 (contact normal/tangents)
    //   - scratch[i].invI (world inertia at start of step)
    //   - row.localA / row.localB (contact in body-local frame)
    // Refreshed each iteration:
    //   - rA/rB via current scratch orientation
    //   - depth via current scratch positions/orientations
    //   - effective mass with new rA/rB
    struct PoseScratch {
        Vector3D    pos;
        Quaternion  orient;
        bool        used = false;   // true when this body has had any NGS displacement
    };
    std::vector<PoseScratch> poseScratch;
    bool ngsAnyActive = false;
    if (cfg.positionIterations > 0) {
        for (const ConstraintRow& r : rows) {
            if (r.ngsActive) { ngsAnyActive = true; break; }
        }
    }
    if (ngsAnyActive) {
        poseScratch.resize(scratch.size());
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i]) {
                const BodyPose p = bodies[i]->GetPose();
                poseScratch[i].pos    = p.position;
                poseScratch[i].orient = p.orientation;
            } else {
                poseScratch[i].pos    = Vector3D(0, 0, 0);
                poseScratch[i].orient = Quaternion();
            }
        }
        // Static slot - pose unused but initialised for safety.
        poseScratch[kStaticIdx].pos    = Vector3D(0, 0, 0);
        poseScratch[kStaticIdx].orient = Quaternion();

        const float beta = cfg.baumgarteBeta;
        const float maxC = cfg.maxLinearCorrection;
        const float slop = cfg.positionSlop;

        for (int iter = 0; iter < cfg.positionIterations; ++iter) {
            for (ConstraintRow& row : rows) {
                if (!row.ngsActive) continue;
                BodyScratch& A = scratch[row.bodyA];
                BodyScratch& B = scratch[row.bodyB];

                // Recompute r-arms from current scratch orientation.
                const Vector3D rA = poseScratch[row.bodyA].orient.RotateVector(row.localA);
                const Vector3D rB = poseScratch[row.bodyB].orient.RotateVector(row.localB);

                // Current world contact points.
                const Vector3D pA = poseScratch[row.bodyA].pos + rA;
                const Vector3D pB = poseScratch[row.bodyB].pos + rB;

                // Current penetration depth along the (frozen) world normal.
                // Derivation: at row build, rA0 / rB0 satisfied (posA0+rA0) ==
                // (posB0+rB0) == c.point (witness midpoint), so initialDepth
                // already encodes the actual overlap. For any subsequent pose
                // displacement, depth_now = initialDepth - (Δ·n) where
                // Δ = (pA - pB) - (pA0 - pB0) = (pA - pB) - 0 = pA - pB.
                // Hence:
                const float depth = row.initialDepth - (pA - pB).DotProduct(row.n);

                const float C = depth - slop;
                if (C <= 0.0f) continue;       // already separated within slop

                // Soften by beta and cap per-iteration correction.
                float Cclamped = beta * C;
                if (Cclamped > maxC) Cclamped = maxC;

                // Recompute effective mass at current r-arms (rotation of body
                // changes the r × n term in the denominator).
                const float effMassN = EffectiveMass(row.n, A, rA, B, rB);
                if (effMassN == 0.0f) continue;

                const float lambda = effMassN * Cclamped;       // > 0 (separation)
                const Vector3D P = row.n * lambda;

                if (A.dynamic) {
                    poseScratch[row.bodyA].pos    = poseScratch[row.bodyA].pos + P * A.invMass;
                    const Vector3D dThetaA        = A.invI.Multiply(rA.CrossProduct(P));
                    if (dThetaA.DotProduct(dThetaA) > 0.0f) {
                        poseScratch[row.bodyA].orient = poseScratch[row.bodyA].orient.DeltaRotation(dThetaA, 1.0f);
                    }
                    poseScratch[row.bodyA].used = true;
                }
                if (B.dynamic) {
                    poseScratch[row.bodyB].pos    = poseScratch[row.bodyB].pos - P * B.invMass;
                    const Vector3D dThetaB        = B.invI.Multiply(rB.CrossProduct(P));
                    if (dThetaB.DotProduct(dThetaB) > 0.0f) {
                        // Negate angular delta on B (impulse applied in -P direction).
                        poseScratch[row.bodyB].orient = poseScratch[row.bodyB].orient.DeltaRotation(dThetaB * -1.0f, 1.0f);
                    }
                    poseScratch[row.bodyB].used = true;
                }
            }
        }
    }


    // ---- Writeback: scratch velocities -> bodies, scratch poses (NGS) -> bodies,
    //      accumulators -> manifolds + cache. ----
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        RigidBody* rb = bodies[i];
        if (!rb || rb->IsStatic() || rb->IsSleeping()) continue;
        if (!scratch[i].dynamic) continue;
        // Direct field writeback (avoids SetVelocity's auto-Wake side-effect, which
        // would defeat sleep islands that we just decided to keep asleep).
        rb->SetVelocityRaw(scratch[i].v);
        rb->SetAngularVelocityRaw(scratch[i].w);

        // NGS position writeback: replace the body's pose with the scratch
        // pose (which has the accumulated penetration corrections). Only
        // applied to bodies that actually participated. Real velocity is
        // unchanged so KE is not affected. PhysicsWorld will subsequently
        // call IntegratePosition(dt) which adds real v*dt on top of this.
        if (ngsAnyActive && poseScratch[i].used) {
            BodyPose pose;
            pose.position    = poseScratch[i].pos;
            pose.orientation = poseScratch[i].orient;
            rb->SetPose(pose);
        }
    }

    for (const ConstraintRow& row : rows) {
        ContactManifold& m = manifolds[row.manifoldIdx];
        Contact& c = m.contacts[row.contactIdx];
        c.accumulatedNormalImpulse     = row.accN;
        c.accumulatedTangentImpulse[0] = row.accT1;
        c.accumulatedTangentImpulse[1] = row.accT2;
        cache.WriteImpulses(m.a, m.b, c.featureId, row.accN, row.accT1, row.accT2);
    }

    // ---- Joint impulse writeback for next-frame warm-start. ----
    for (std::size_t ji = 0; ji < joints.size(); ++ji) {
        Joint* j = joints[ji];
        if (!j) continue;
        const int n     = jointRowCounts.empty() ? 0 : jointRowCounts[ji];
        const int start = jointRowStart.empty()  ? 0 : jointRowStart[ji];
        if (n <= 0) {
            j->StoreImpulses(nullptr, 0);
            continue;
        }
        JointRow tmp[Joint::kMaxJointRows];
        for (int r = 0; r < n; ++r) tmp[r] = jointRows[start + r].row;
        j->StoreImpulses(tmp, n);
    }
}

} // namespace koilo
