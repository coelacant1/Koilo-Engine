// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <algorithm>
#include <cmath>

namespace koilo {

PhysicsWorld::PhysicsWorld()
    : gravity_(0, -9.81f, 0), fixedDt_(1.0f / 60.0f),
      accumulator_(0.0f), maxSubSteps_(8) {
}

PhysicsWorld::PhysicsWorld(const Vector3D& gravity)
    : gravity_(gravity), fixedDt_(1.0f / 60.0f),
      accumulator_(0.0f), maxSubSteps_(8) {
}

// === Body Management ===

void PhysicsWorld::AddBody(RigidBody* body) {
    if (body == nullptr) return;
    auto it = std::find(bodies_.begin(), bodies_.end(), body);
    if (it == bodies_.end()) {
        bodies_.push_back(body);
        if (body->GetCollider()) {
            collisionManager_.RegisterCollider(body->GetCollider());
        }
    }
}

void PhysicsWorld::RemoveBody(RigidBody* body) {
    if (body == nullptr) return;
    if (body->GetCollider()) {
        collisionManager_.UnregisterCollider(body->GetCollider());
    }
    bodies_.erase(std::remove(bodies_.begin(), bodies_.end(), body), bodies_.end());
}

void PhysicsWorld::RemoveAllBodies() {
    collisionManager_.UnregisterAllColliders();
    bodies_.clear();
    previousCollisions_.clear();
    currentCollisions_.clear();
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

// === Simulation ===

void PhysicsWorld::Step() {
    float frameDt = TimeManager::GetInstance().GetDeltaTime();
    if (fixedDt_ <= 0.0f) return;

    accumulator_ += frameDt;

    // Clamp to prevent spiral of death
    float maxAccumulator = fixedDt_ * maxSubSteps_;
    if (accumulator_ > maxAccumulator) {
        accumulator_ = maxAccumulator;
    }

    while (accumulator_ >= fixedDt_) {
        FixedStep();
        accumulator_ -= fixedDt_;
    }
}

void PhysicsWorld::FixedStep() {
    // 1. Integrate all dynamic bodies
    for (auto* body : bodies_) {
        body->Integrate(fixedDt_, gravity_);
    }

    // 2. Detect collisions and track enter/stay/exit
    currentCollisions_.clear();
    debugContacts_.clear();

    size_t n = bodies_.size();
    for (size_t i = 0; i < n; ++i) {
        Collider* ci = bodies_[i]->GetCollider();
        if (!ci || !ci->IsEnabled()) continue;

        for (size_t j = i + 1; j < n; ++j) {
            Collider* cj = bodies_[j]->GetCollider();
            if (!cj || !cj->IsEnabled()) continue;

            if (!collisionManager_.CanLayersCollide(ci->GetLayer(), cj->GetLayer())) {
                continue;
            }

            CollisionInfo info;
            if (collisionManager_.TestCollision(ci, cj, info)) {
                uint64_t pairID = GetPairID(ci, cj);
                currentCollisions_.insert(pairID);

                // 3. Resolve collision with impulse
                ResolveCollision(bodies_[i], bodies_[j], info);
                // 4. Correct penetration
                CorrectPenetration(bodies_[i], bodies_[j], info);

                // Store for debug visualization
                {
                    CollisionEvent dc;
                    dc.colliderA = info.colliderA;
                    dc.colliderB = info.colliderB;
                    dc.bodyA = bodies_[i];
                    dc.bodyB = bodies_[j];
                    dc.contactPoint = info.contactPoint;
                    dc.normal = info.normal;
                    dc.penetration = info.penetrationDepth;
                    debugContacts_.push_back(dc);
                }

                // 5. Fire enter or stay callbacks
                if (previousCollisions_.find(pairID) == previousCollisions_.end()) {
                    FireCollisionEvent(bodies_[i], bodies_[j], info, onEnterCallbacks_);
                } else {
                    FireCollisionEvent(bodies_[i], bodies_[j], info, onStayCallbacks_);
                }
            }
        }
    }

    // 6. Fire exit callbacks for collisions that ended
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
        if (body->GetCollider() == collider) return body;
    }
    return nullptr;
}

void PhysicsWorld::ResolveCollision(RigidBody* a, RigidBody* b, const CollisionInfo& info) {
    float invMassA = a->GetInverseMass();
    float invMassB = b->GetInverseMass();
    float invMassSum = invMassA + invMassB;

    if (invMassSum <= 0.0f) return;

    Vector3D relativeVelocity = a->GetVelocity() - b->GetVelocity();
    float velAlongNormal = relativeVelocity.DotProduct(info.normal);

    // Bodies separating - no impulse needed
    // With normal from A to B: positive velAlongNormal = approaching
    if (velAlongNormal < 0.0f) return;

    float e = std::min(a->GetRestitution(), b->GetRestitution());

    float j = -(1.0f + e) * velAlongNormal / invMassSum;

    Vector3D impulse = info.normal * j;
    a->SetVelocity(a->GetVelocity() + impulse * invMassA);
    b->SetVelocity(b->GetVelocity() - impulse * invMassB);

    // Friction impulse
    relativeVelocity = a->GetVelocity() - b->GetVelocity();
    Vector3D tangent = relativeVelocity - info.normal * relativeVelocity.DotProduct(info.normal);
    float tangentMag = tangent.Magnitude();

    if (tangentMag > 1e-6f) {
        tangent = tangent * (1.0f / tangentMag);

        float jt = -relativeVelocity.DotProduct(tangent) / invMassSum;

        float mu = std::sqrt(a->GetFriction() * b->GetFriction());
        Vector3D frictionImpulse;
        if (std::abs(jt) < j * mu) {
            frictionImpulse = tangent * jt;
        } else {
            frictionImpulse = tangent * (-j * mu);
        }

        a->SetVelocity(a->GetVelocity() + frictionImpulse * invMassA);
        b->SetVelocity(b->GetVelocity() - frictionImpulse * invMassB);
    }
}

void PhysicsWorld::CorrectPenetration(RigidBody* a, RigidBody* b, const CollisionInfo& info) {
    float invMassA = a->GetInverseMass();
    float invMassB = b->GetInverseMass();
    float invMassSum = invMassA + invMassB;

    if (invMassSum <= 0.0f) return;

    const float correctionPercent = 0.4f;
    const float slop = 0.01f;

    float correctionMag = std::max(info.penetrationDepth - slop, 0.0f) * correctionPercent / invMassSum;
    Vector3D correction = info.normal * correctionMag;

    if (a->GetCollider()) {
        Vector3D posA = a->GetCollider()->GetPosition();
        a->GetCollider()->SetPosition(posA + correction * invMassA);
    }
    if (b->GetCollider()) {
        Vector3D posB = b->GetCollider()->GetPosition();
        b->GetCollider()->SetPosition(posB - correction * invMassB);
    }
}

} // namespace koilo
