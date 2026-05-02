// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/physicsworld.hpp>

#include <koilo/systems/physics/contactmanifold.hpp>
#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/systems/physics/meshcollider.hpp>
#include <koilo/systems/physics/heightfieldcollider.hpp>
#include <koilo/systems/physics/shape/sphereshape.hpp>
#include <koilo/systems/physics/shape/boxshape.hpp>
#include <koilo/systems/physics/shape/capsuleshape.hpp>
#include <koilo/systems/physics/shape/planeshape.hpp>
#include <koilo/systems/physics/shape/trianglemeshshape.hpp>
#include <koilo/systems/physics/shape/heightfieldshape.hpp>
#include <koilo/systems/physics/narrowphase/manifoldgenerator.hpp>
#include <koilo/systems/physics/solver/islandbuilder.hpp>
#include <koilo/systems/physics/joints/joint.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace koilo {

PhysicsWorld::PhysicsWorld()
    : gravity_(0, -9.81f, 0), fixedDt_(1.0f / 120.0f),
      accumulator_(0.0f), maxSubSteps_(4),
      budget_(), transformPolicy_(PhysicsTransformPolicy::PhysicsAuthoritative),
      determinismTier_(DeterminismTier::T1_SameBinaryReplay),
      interpolationAlpha_(0.0f), lastStepSubsteps_(0), lastStepOverran_(false) {
}

PhysicsWorld::PhysicsWorld(const Vector3D& gravity)
    : gravity_(gravity), fixedDt_(1.0f / 120.0f),
      accumulator_(0.0f), maxSubSteps_(4),
      budget_(), transformPolicy_(PhysicsTransformPolicy::PhysicsAuthoritative),
      determinismTier_(DeterminismTier::T1_SameBinaryReplay),
      interpolationAlpha_(0.0f), lastStepSubsteps_(0), lastStepOverran_(false) {
}

// === Body Management ===

void PhysicsWorld::AddBody(RigidBody* body) {
    if (body == nullptr) return;
    auto it = std::find(bodies_.begin(), bodies_.end(), body);
    if (it != bodies_.end()) return;

    bodies_.push_back(body);
    shapes_.emplace_back(nullptr);
    proxies_.emplace_back(nullptr);

    if (body->GetCollider()) {
        collisionManager_.RegisterCollider(body->GetCollider());
    }
    RegisterProxyForBody(bodies_.size() - 1);
}

void PhysicsWorld::RemoveBody(RigidBody* body) {
    if (body == nullptr) return;
    auto it = std::find(bodies_.begin(), bodies_.end(), body);
    if (it == bodies_.end()) return;
    const std::size_t idx = static_cast<std::size_t>(it - bodies_.begin());

    // Auto-detach any joints referencing this body to prevent dangling pointers.
    joints_.erase(std::remove_if(joints_.begin(), joints_.end(),
                                 [body](Joint* j) {
                                     return j && (j->GetBodyA() == body || j->GetBodyB() == body);
                                 }),
                  joints_.end());

    if (body->GetCollider()) {
        collisionManager_.UnregisterCollider(body->GetCollider());
    }
    if (proxies_[idx]) {
        broadphase_.Remove(proxies_[idx].get());
    }
    bodies_.erase(bodies_.begin() + idx);
    shapes_.erase(shapes_.begin() + idx);
    proxies_.erase(proxies_.begin() + idx);

    RebuildProxyBodyIds();
    contactCache_.Clear();         // pointers may now be stale; rebuild next step
}

void PhysicsWorld::AddJoint(Joint* joint) {
    if (!joint) return;
    if (std::find(joints_.begin(), joints_.end(), joint) != joints_.end()) return;
    joints_.push_back(joint);
}

void PhysicsWorld::RemoveJoint(Joint* joint) {
    if (!joint) return;
    auto it = std::find(joints_.begin(), joints_.end(), joint);
    if (it != joints_.end()) joints_.erase(it);
}

void PhysicsWorld::RemoveAllBodies() {
    for (auto& p : proxies_) {
        if (p) broadphase_.Remove(p.get());
    }
    collisionManager_.UnregisterAllColliders();
    bodies_.clear();
    shapes_.clear();
    proxies_.clear();
    previousCollisions_.clear();
    currentCollisions_.clear();
    contactCache_.Clear();
}

std::unique_ptr<IShape> PhysicsWorld::BuildShapeFromCollider(Collider* collider) const {
    if (!collider) return nullptr;
    switch (collider->GetType()) {
        case ColliderType::Sphere: {
            auto* sc = static_cast<SphereCollider*>(collider);
            return std::make_unique<SphereShape>(sc->GetRadius());
        }
        case ColliderType::Box: {
            auto* bc = static_cast<BoxCollider*>(collider);
            const Vector3D size = bc->GetSize();
            return std::make_unique<BoxShape>(Vector3D(size.X * 0.5f, size.Y * 0.5f, size.Z * 0.5f));
        }
        case ColliderType::Capsule: {
            auto* cc = static_cast<CapsuleCollider*>(collider);
            const float r = cc->GetRadius();
            const float halfH = std::max(0.0f, cc->GetHeight() * 0.5f - r);
            return std::make_unique<CapsuleShape>(r, halfH);
        }
        case ColliderType::Mesh: {
            auto* mc = static_cast<MeshCollider*>(collider);
            auto data = mc->GetData();
            if (!data || data->Empty()) return nullptr;
            return std::make_unique<TriangleMeshShape>(data);
        }
        case ColliderType::Heightfield: {
            auto* hc = static_cast<HeightfieldCollider*>(collider);
            auto data = hc->GetData();
            if (!data || data->Empty()) return nullptr;
            return std::make_unique<HeightfieldShape>(data);
        }
        case ColliderType::Plane:
        case ColliderType::Custom:
        default:
            return nullptr;
    }
}

void PhysicsWorld::RegisterProxyForBody(std::size_t index) {
    if (index >= bodies_.size()) return;
    RigidBody* body = bodies_[index];
    if (!body) return;
    Collider* collider = body->GetCollider();
    auto shape = BuildShapeFromCollider(collider);
    if (!shape) return;

    auto proxy = std::make_unique<ColliderProxy>();
    proxy->shape   = shape.get();
    proxy->layer   = collider ? static_cast<std::uint32_t>(collider->GetLayer()) : 0u;
    proxy->bodyId  = static_cast<std::uint32_t>(index);
    proxy->isTrigger = collider ? collider->IsTrigger() : false;
    proxy->enabled   = collider ? collider->IsEnabled() : true;
    proxy->localOffset = collider ? collider->GetLocalOffset() : BodyPose();

    broadphase_.Add(proxy.get(), body->GetPose());
    shapes_[index]  = std::move(shape);
    proxies_[index] = std::move(proxy);
}

void PhysicsWorld::RebuildProxyBodyIds() {
    for (std::size_t i = 0; i < proxies_.size(); ++i) {
        if (proxies_[i]) proxies_[i]->bodyId = static_cast<std::uint32_t>(i);
    }
}

// === Collision Callbacks ===

void PhysicsWorld::OnCollisionEnter(PhysicsCollisionCallback callback) {
    onEnterCallbacks_.push_back(callback);
}

void PhysicsWorld::OnCollisionStay(PhysicsCollisionCallback callback) {
    onStayCallbacks_.push_back(callback);
}

void PhysicsWorld::OnCollisionExit(PhysicsCollisionCallback callback) {
    onExitCallbacks_.push_back(callback);
}

void PhysicsWorld::ClearCollisionCallbacks() {
    onEnterCallbacks_.clear();
    onStayCallbacks_.clear();
    onExitCallbacks_.clear();
}

PreFixedStepCallbackId
PhysicsWorld::RegisterPreFixedStepCallback(PreFixedStepCallback cb) {
    if (!cb) return kInvalidPreFixedStepCallbackId;
    const PreFixedStepCallbackId id = nextPreFixedStepId_++;
    if (nextPreFixedStepId_ == kInvalidPreFixedStepCallbackId) {
        // Wrap-around guard: skip the reserved sentinel.
        nextPreFixedStepId_ = 1;
    }
    preFixedStepCallbacks_.push_back({id, std::move(cb)});
    return id;
}

void PhysicsWorld::UnregisterPreFixedStepCallback(PreFixedStepCallbackId id) {
    if (id == kInvalidPreFixedStepCallbackId) return;
    for (auto it = preFixedStepCallbacks_.begin(); it != preFixedStepCallbacks_.end(); ++it) {
        if (it->id == id) {
            preFixedStepCallbacks_.erase(it);
            return;
        }
    }
}

void PhysicsWorld::ClearPreFixedStepCallbacks() {
    preFixedStepCallbacks_.clear();
}

// === Simulation ===

void PhysicsWorld::Step(float frameDt) {
    lastStepSubsteps_ = 0;
    lastStepOverran_ = false;
    interpolationAlpha_ = 0.0f;

    if (fixedDt_ <= 0.0f) return;
    if (frameDt < 0.0f) frameDt = 0.0f;

    accumulator_ += frameDt;

    const int substepCap = (budget_.maxSubsteps > 0) ? budget_.maxSubsteps : maxSubSteps_;
    const float maxAccumulator = fixedDt_ * static_cast<float>(substepCap);
    if (accumulator_ > maxAccumulator) {
        accumulator_ = maxAccumulator;
        lastStepOverran_ = true;
    }

    const auto stepStart = std::chrono::steady_clock::now();
    const float maxStepMs = budget_.maxStepMs;

    while (accumulator_ >= fixedDt_ && lastStepSubsteps_ < substepCap) {
        FixedStep();
        accumulator_ -= fixedDt_;
        ++lastStepSubsteps_;

        if (maxStepMs > 0.0f) {
            const auto elapsed = std::chrono::steady_clock::now() - stepStart;
            const float elapsedMs = std::chrono::duration<float, std::milli>(elapsed).count();
            if (elapsedMs >= maxStepMs) {
                lastStepOverran_ = true;
                accumulator_ = 0.0f;
                break;
            }
        }
    }

    interpolationAlpha_ = (fixedDt_ > 0.0f) ? (accumulator_ / fixedDt_) : 0.0f;
}

BodyPose PhysicsWorld::GetInterpolatedPose(const RigidBody* body, float alpha) const {
    if (!body) return BodyPose();
    if (alpha <= 0.0f) return body->GetPreviousPose();
    if (alpha >= 1.0f) return body->GetPose();
    return BodyPose::Interpolate(body->GetPreviousPose(), body->GetPose(), alpha);
}

void PhysicsWorld::FixedStep() {
    // 0. Snapshot current pose -> previousPose for render-side interpolation.
    for (auto* body : bodies_) {
        if (body) body->SnapshotPose();
    }

    // 1. Velocity integration only (apply gravity + accumulated forces). Pose
    //    is NOT advanced yet. This lets the solver clamp velocities against
    //    contact constraints before they translate into position changes -
    //    the standard PGS pipeline order (Bullet/Box2D/PhysX) and the
    //    prerequisite for CCD / speculative contacts.
    //
    //    Pre-fixed-step callbacks (e.g. aerodynamics force injection) run
    //    BEFORE integration so any forces they apply are picked up this tick.
    for (const auto& entry : preFixedStepCallbacks_) {
        if (entry.fn) entry.fn(fixedDt_);
    }
    for (auto* body : bodies_) {
        if (body) body->IntegrateVelocity(fixedDt_, gravity_);
    }

    // 2. Refresh broadphase AABBs from current poses (pre-integration). For
    //    `bullet` bodies, inflate the broadphase margin by the
    //    swept motion this substep so fast-moving proxies still discover
    //    pairs that they'd tunnel past at integration time. The DBVT only
    //    re-fits when the new AABB exits the inflated leaf, so non-bullet
    //    bodies pay the same cost as before; bullet bodies pay an extra
    //    re-fit when motion exceeds `baseBroadphaseMargin_`.
    lastStepBulletProxies_ = 0;
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        if (!bodies_[i] || !proxies_[i]) continue;
        if (bodies_[i]->IsBullet()) {
            // Refresh worldAabb at the current pose first so we can derive a
            // tight bounding radius (max distance from COM to AABB corner -
            // this implicitly includes the proxy's localOffset because the
            // shape's WorldAABB is composed against it).
            proxies_[i]->RefreshWorldAABB(bodies_[i]->GetPose());
            const Vector3D com    = bodies_[i]->GetPose().position;
            const Vector3D mn     = proxies_[i]->worldAabb.min;
            const Vector3D mx     = proxies_[i]->worldAabb.max;
            const float dxA = std::max(std::fabs(mn.X - com.X), std::fabs(mx.X - com.X));
            const float dyA = std::max(std::fabs(mn.Y - com.Y), std::fabs(mx.Y - com.Y));
            const float dzA = std::max(std::fabs(mn.Z - com.Z), std::fabs(mx.Z - com.Z));
            const float radius = std::sqrt(dxA*dxA + dyA*dyA + dzA*dzA);

            const float linSpeed = bodies_[i]->GetVelocity().Magnitude();
            const float angSpeed = bodies_[i]->GetAngularVelocity().Magnitude();
            float sweptInflate = (linSpeed + angSpeed * radius) * fixedDt_;
            if (sweptInflate > maxSweptMargin_) sweptInflate = maxSweptMargin_;
            broadphase_.UpdateInflated(proxies_[i].get(), bodies_[i]->GetPose(),
                                       sweptInflate, baseBroadphaseMargin_);
            ++lastStepBulletProxies_;
        } else {
            broadphase_.Update(proxies_[i].get(), bodies_[i]->GetPose(), baseBroadphaseMargin_);
        }
    }

    // 3. Broadphase: collect overlapping proxy pairs (sorted, deterministic).
    auto pairs = broadphase_.CollectPairs();

    // 4. Narrowphase: generate manifolds, populate contact cache, fire enter/stay.
    contactCache_.BeginFrame();
    currentCollisions_.clear();
    debugContacts_.clear();
    lastStepSpeculativeManifolds_ = 0;

    std::vector<ContactManifold> manifolds;
    manifolds.reserve(pairs.size());

    for (auto& pp : pairs) {
        ColliderProxy* pa = pp.first;
        ColliderProxy* pb = pp.second;
        if (!pa || !pb) continue;
        if (!pa->enabled || !pb->enabled) continue;

        const std::uint32_t ia = pa->bodyId;
        const std::uint32_t ib = pb->bodyId;
        if (ia >= bodies_.size() || ib >= bodies_.size()) continue;

        RigidBody* ba = bodies_[ia];
        RigidBody* bb = bodies_[ib];
        if (!ba || !bb) continue;

        Collider* ca = ba->GetCollider();
        Collider* cb = bb->GetCollider();
        if (!ca || !cb) continue;
        if (!ca->IsEnabled() || !cb->IsEnabled()) continue;
        if (!collisionManager_.CanLayersCollide(ca->GetLayer(), cb->GetLayer())) continue;

        ContactManifold m;
        bool generated = ManifoldGenerator::Generate(pa, ba->GetPose(), pb, bb->GetPose(), m);

        // speculative-contact fallback for bullet pairs that did not
        // overlap. Closing-speed estimate uses linear COM velocities plus an
        // angular-bound from each proxy's worldAabb-derived bounding radius
        // (over-approximation: safe - only widens the margin, never misses).
        const bool bulletPair = (ba->IsBullet() || bb->IsBullet());
        const bool isTrigger = ca->IsTrigger() || cb->IsTrigger();
        if (!generated && bulletPair && !isTrigger) {
            // Conservative bounding radius from refreshed worldAabb corners.
            auto BoundRadius = [](const ColliderProxy& p, const Vector3D& com) {
                const Vector3D mn = p.worldAabb.min;
                const Vector3D mx = p.worldAabb.max;
                const float dx = std::max(std::fabs(mn.X - com.X), std::fabs(mx.X - com.X));
                const float dy = std::max(std::fabs(mn.Y - com.Y), std::fabs(mx.Y - com.Y));
                const float dz = std::max(std::fabs(mn.Z - com.Z), std::fabs(mx.Z - com.Z));
                return std::sqrt(dx*dx + dy*dy + dz*dz);
            };
            const float rA = BoundRadius(*pa, ba->GetPose().position);
            const float rB = BoundRadius(*pb, bb->GetPose().position);
            const Vector3D vRel = ba->GetVelocity() - bb->GetVelocity();
            const float vRelMag = vRel.Magnitude();
            const float angContribA = ba->GetAngularVelocity().Magnitude() * rA;
            const float angContribB = bb->GetAngularVelocity().Magnitude() * rB;
            const float closingMax = vRelMag + angContribA + angContribB;
            const float speculativeMargin = solverConfig_.positionSlop + closingMax * fixedDt_;
            ContactManifold ms;
            if (ManifoldGenerator::GenerateSpeculative(pa, ba->GetPose(), pb, bb->GetPose(),
                                                       speculativeMargin, ms)) {
                m = ms;
                generated = true;
                ++lastStepSpeculativeManifolds_;
            }
        }

        if (!generated) continue;

        contactCache_.Touch(m);

        if (!isTrigger) {
            manifolds.push_back(m);
        }

        // Speculative manifolds intentionally skip the high-level collision
        // event bookkeeping: bodies haven't actually touched yet, so no
        // OnCollisionEnter/Stay/Exit should fire and they must not appear in
        // currentCollisions_ (otherwise next step's transition logic gets
        // confused). They still feed the solver so the bullet motion gets
        // arrested at the gap.
        if (m.isSpeculative) {
            CollisionEvent dc;
            dc.colliderA = ca;
            dc.colliderB = cb;
            dc.bodyA = ba;
            dc.bodyB = bb;
            const CollisionInfo info = m.ToCollisionInfo(ca, cb);
            dc.contactPoint = info.contactPoint;
            dc.normal = info.normal;
            dc.penetration = info.penetrationDepth; // negative for speculative
            debugContacts_.push_back(dc);
            continue;
        }

        const uint64_t pairID = GetPairID(ca, cb);
        currentCollisions_.insert(pairID);

        const CollisionInfo info = m.ToCollisionInfo(ca, cb);

        CollisionEvent dc;
        dc.colliderA = ca;
        dc.colliderB = cb;
        dc.bodyA = ba;
        dc.bodyB = bb;
        dc.contactPoint = info.contactPoint;
        dc.normal = info.normal;
        dc.penetration = info.penetrationDepth;
        debugContacts_.push_back(dc);

        if (previousCollisions_.find(pairID) == previousCollisions_.end()) {
            FireCollisionEvent(ba, bb, info, onEnterCallbacks_);
        } else {
            FireCollisionEvent(ba, bb, info, onStayCallbacks_);
        }
    }

    contactCache_.EndFrame();

    // 5. Wake-pass: build islands treating sleeping bodies as bridges so a
    //    moving body can wake an entire chain of currently-sleeping intermediates.
    {
        const auto bridgeIslands =
            IslandBuilder::Build(manifolds, joints_, bodies_, /*treatSleepingAsBridge=*/true);
        SleepManager::WakeIslandsWithMotion(bridgeIslands, bodies_, sleepConfig_);
    }

    // 6. Solver: iteratively resolve normal + friction impulses with Baumgarte
    //    velocity bias for positional error. Sleeping bodies act as immovable
    //    walls (handled inside the solver's scratch builder).
    if (!manifolds.empty() || !joints_.empty()) {
        solver_.Solve(manifolds, bodies_, joints_, contactCache_, fixedDt_, solverConfig_);
    }

    // 7. Position integration using solver-corrected velocities. This is what
    //    actually moves the bodies; running it after the solver is what lets
    //    speculative contacts prevent tunneling.
    for (auto* body : bodies_) {
        if (body) body->IntegratePosition(fixedDt_);
    }

    // 8. Sleep-decision pass: anchored sleeping bodies; any island where every
    //    member has stayed below sleepEnergyThreshold for sleepTimeRequired
    //    seconds is put to sleep together.
    {
        const auto sleepIslands =
            IslandBuilder::Build(manifolds, joints_, bodies_, /*treatSleepingAsBridge=*/false);
        SleepManager::AttemptSleep(sleepIslands, bodies_, fixedDt_, sleepConfig_);
    }

    // 9. Fire exit callbacks for collisions that ended this step.
    if (!onExitCallbacks_.empty()) {
        for (uint64_t pairID : previousCollisions_) {
            if (currentCollisions_.find(pairID) == currentCollisions_.end()) {
                CollisionEvent evt;
                for (auto& cb : onExitCallbacks_) {
                    cb(evt);
                }
            }
        }
    }

    previousCollisions_ = currentCollisions_;
}

uint64_t PhysicsWorld::GetPairID(Collider* a, Collider* b) const {
    uintptr_t ptrA = reinterpret_cast<uintptr_t>(a);
    uintptr_t ptrB = reinterpret_cast<uintptr_t>(b);
    if (ptrA > ptrB) std::swap(ptrA, ptrB);
    return (static_cast<uint64_t>(ptrA) << 32) | static_cast<uint64_t>(ptrB);
}

void PhysicsWorld::FireCollisionEvent(RigidBody* a, RigidBody* b, const CollisionInfo& info,
                                       const std::vector<PhysicsCollisionCallback>& callbacks) {
    if (callbacks.empty()) return;

    CollisionEvent evt;
    evt.colliderA = info.colliderA;
    evt.colliderB = info.colliderB;
    evt.bodyA = a;
    evt.bodyB = b;
    evt.contactPoint = info.contactPoint;
    evt.normal = info.normal;
    evt.penetration = info.penetrationDepth;

    for (auto& cb : callbacks) {
        cb(evt);
    }
}

RigidBody* PhysicsWorld::FindBodyForCollider(Collider* collider) const {
    for (auto* body : bodies_) {
        if (body && body->GetCollider() == collider) return body;
    }
    return nullptr;
}

PhysicsDiagnostics PhysicsWorld::ComputeDiagnostics() const {
    PhysicsDiagnostics d{};
    for (auto* body : bodies_) {
        if (!body || !body->IsDynamic()) continue;
        const float m = body->GetMass();
        const Vector3D& v = body->GetVelocity();
        const Vector3D& w = body->GetAngularVelocity();
        const Vector3D& r = body->GetPose().position;
        const Quaternion& q = body->GetPose().orientation;

        // Linear: P += m*v
        const Vector3D mv(m * v.X, m * v.Y, m * v.Z);
        d.linearMomentum = d.linearMomentum + mv;

        // Angular momentum in world frame:
        //   L_world = R * I_local * R^T * w
        // Using quaternion: w_local = q^-1 * w; L_local = I_local * w_local; L_world = q * L_local.
        const Vector3D wLocal = q.UnrotateVector(w);
        const Vector3D LLocal = body->GetInertiaLocal().Multiply(wLocal);
        const Vector3D LWorld = q.RotateVector(LLocal);

        // L_total += r × (m v) + L_world
        d.angularMomentum = d.angularMomentum + r.CrossProduct(mv) + LWorld;

        // KE = 0.5*m*|v|^2 + 0.5*w·L_world
        const float vSq = v.X*v.X + v.Y*v.Y + v.Z*v.Z;
        const float wDotL = w.X*LWorld.X + w.Y*LWorld.Y + w.Z*LWorld.Z;
        d.kineticEnergy += 0.5f * m * vSq + 0.5f * wDotL;
        ++d.bodyCount;
    }
    return d;
}

} // namespace koilo
