// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/collider.hpp>

namespace koilo {

RigidBody::RigidBody()
    : bodyType_(BodyType::Dynamic), mass_(1.0f), inverseMass_(1.0f),
      restitution_(0.3f), friction_(0.5f), linearDamping_(0.01f),
      velocity_(0, 0, 0), forceAccumulator_(0, 0, 0), collider_(nullptr) {
}

RigidBody::RigidBody(BodyType type, float mass)
    : bodyType_(type), mass_(mass), inverseMass_(0.0f),
      restitution_(0.3f), friction_(0.5f), linearDamping_(0.01f),
      velocity_(0, 0, 0), forceAccumulator_(0, 0, 0), collider_(nullptr) {
    if (type == BodyType::Dynamic && mass > 0.0f) {
        inverseMass_ = 1.0f / mass;
    }
}

void RigidBody::SetBodyType(BodyType type) {
    bodyType_ = type;
    if (type != BodyType::Dynamic) {
        inverseMass_ = 0.0f;
        velocity_ = Vector3D(0, 0, 0);
        forceAccumulator_ = Vector3D(0, 0, 0);
    } else if (mass_ > 0.0f) {
        inverseMass_ = 1.0f / mass_;
    }
}

void RigidBody::SetMass(float mass) {
    mass_ = mass;
    if (bodyType_ == BodyType::Dynamic && mass > 0.0f) {
        inverseMass_ = 1.0f / mass;
    } else {
        inverseMass_ = 0.0f;
    }
}

void RigidBody::ApplyForce(const Vector3D& force) {
    if (bodyType_ != BodyType::Dynamic) return;
    forceAccumulator_ = forceAccumulator_ + force;
}

void RigidBody::ApplyImpulse(const Vector3D& impulse) {
    if (bodyType_ != BodyType::Dynamic) return;
    velocity_ = velocity_ + impulse * inverseMass_;
}

void RigidBody::ClearForces() {
    forceAccumulator_ = Vector3D(0, 0, 0);
}

void RigidBody::Integrate(float dt, const Vector3D& gravity) {
    if (bodyType_ != BodyType::Dynamic) return;
    if (inverseMass_ <= 0.0f) return;

    // Semi-implicit Euler: update velocity first, then position
    // Apply gravity + accumulated forces
    Vector3D acceleration = gravity + forceAccumulator_ * inverseMass_;
    velocity_ = velocity_ + acceleration * dt;

    // Apply linear damping
    velocity_ = velocity_ * (1.0f - linearDamping_ * dt);

    // Update collider position
    if (collider_) {
        Vector3D pos = collider_->GetPosition();
        pos = pos + velocity_ * dt;
        collider_->SetPosition(pos);
    }

    ClearForces();
}

} // namespace koilo
