// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/collider.hpp>
#include <koilo/systems/physics/inertiatensor.hpp>

namespace koilo {

namespace {
inline Matrix3x3 Zero3x3() { return Matrix3x3::Diagonal(Vector3D(0,0,0)); }
}

RigidBody::RigidBody()
    : bodyType_(BodyType::Dynamic), mass_(1.0f), inverseMass_(1.0f),
      restitution_(0.3f), friction_(0.5f), linearDamping_(0.01f),
      velocity_(0, 0, 0), forceAccumulator_(0, 0, 0),
      angularVelocity_(0, 0, 0), torqueAccumulator_(0, 0, 0),
      inertiaLocal_(Zero3x3()), invInertiaLocal_(Zero3x3()),
      invInertiaWorld_(Zero3x3()), angularDamping_(0.05f),
      collider_(nullptr), pose_(), previousPose_() {
}

RigidBody::RigidBody(BodyType type, float mass)
    : bodyType_(type), mass_(mass), inverseMass_(0.0f),
      restitution_(0.3f), friction_(0.5f), linearDamping_(0.01f),
      velocity_(0, 0, 0), forceAccumulator_(0, 0, 0),
      angularVelocity_(0, 0, 0), torqueAccumulator_(0, 0, 0),
      inertiaLocal_(Zero3x3()), invInertiaLocal_(Zero3x3()),
      invInertiaWorld_(Zero3x3()), angularDamping_(0.05f),
      collider_(nullptr), pose_(), previousPose_() {
    if (type == BodyType::Dynamic && mass > 0.0f) {
        inverseMass_ = 1.0f / mass;
    }
}

void RigidBody::SetCollider(Collider* c) {
    collider_ = c;
    // Seed the body pose from the collider so existing call patterns
    // (configure collider position, then attach to body) keep working.
    if (collider_) {
        pose_.position = collider_->GetPosition();
        previousPose_ = pose_;
    }
}

void RigidBody::SetPose(const BodyPose& pose) {
    pose_ = pose;
    if (collider_) {
        // collision math reads collider position; mirror authoritative pose into it.
        // localOffset_ orientation/translation is identity-by-default; full Compose()
        // applies once angular state is live.
        collider_->SetPosition(pose_.position + pose_.orientation.RotateVector(collider_->GetLocalOffset().position));
    }
}

void RigidBody::SetBodyType(BodyType type) {
    bodyType_ = type;
    if (type != BodyType::Dynamic) {
        inverseMass_ = 0.0f;
        velocity_ = Vector3D(0, 0, 0);
        angularVelocity_ = Vector3D(0, 0, 0);
        forceAccumulator_ = Vector3D(0, 0, 0);
        torqueAccumulator_ = Vector3D(0, 0, 0);
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
    Wake();
}

void RigidBody::ApplyImpulse(const Vector3D& impulse) {
    if (bodyType_ != BodyType::Dynamic) return;
    velocity_ = velocity_ + impulse * inverseMass_;
    Wake();
}

void RigidBody::ApplyTorque(const Vector3D& torque) {
    if (bodyType_ != BodyType::Dynamic) return;
    torqueAccumulator_ = torqueAccumulator_ + torque;
    Wake();
}

void RigidBody::ApplyForceAtPoint(const Vector3D& force, const Vector3D& worldPoint) {
    if (bodyType_ != BodyType::Dynamic) return;
    forceAccumulator_ = forceAccumulator_ + force;
    const Vector3D r = worldPoint - pose_.position;
    torqueAccumulator_ = torqueAccumulator_ + r.CrossProduct(force);
    Wake();
}

void RigidBody::ApplyAngularImpulse(const Vector3D& angularImpulse) {
    if (bodyType_ != BodyType::Dynamic) return;
    // Use world-space inverse inertia derived from current orientation.
    const Matrix3x3 R = Matrix3x3::FromQuaternion(pose_.orientation);
    const Matrix3x3 invIw = R.Multiply(invInertiaLocal_).Multiply(R.Transpose());
    angularVelocity_ = angularVelocity_ + invIw.Multiply(angularImpulse);
    Wake();
}

Vector3D RigidBody::GetPointVelocity(const Vector3D& worldPoint) const {
    const Vector3D r = worldPoint - pose_.position;
    return velocity_ + angularVelocity_.CrossProduct(r);
}

void RigidBody::SetInertiaLocal(const Matrix3x3& I) {
    inertiaLocal_ = I;
    invInertiaLocal_ = InertiaTensor::Invert(I);
    const Matrix3x3 R = Matrix3x3::FromQuaternion(pose_.orientation);
    invInertiaWorld_ = R.Multiply(invInertiaLocal_).Multiply(R.Transpose());
}

void RigidBody::SetInertiaSphere(float radius) {
    SetInertiaLocal(InertiaTensor::SolidSphere(mass_, radius));
}
void RigidBody::SetInertiaBox(const Vector3D& halfExtents) {
    SetInertiaLocal(InertiaTensor::SolidBox(mass_, halfExtents));
}
void RigidBody::SetInertiaCapsule(float radius, float halfHeight) {
    SetInertiaLocal(InertiaTensor::SolidCapsule(mass_, radius, halfHeight));
}
void RigidBody::SetInertiaCylinder(float radius, float halfHeight) {
    SetInertiaLocal(InertiaTensor::SolidCylinder(mass_, radius, halfHeight));
}

void RigidBody::ClearForces() {
    forceAccumulator_ = Vector3D(0, 0, 0);
    torqueAccumulator_ = Vector3D(0, 0, 0);
}

void RigidBody::IntegrateVelocity(float dt, const Vector3D& gravity) {
    if (bodyType_ != BodyType::Dynamic) return;
    if (inverseMass_ <= 0.0f) return;
    if (sleeping_) return;

    // === Linear (semi-implicit Euler velocity step) ===
    Vector3D acceleration = gravity + forceAccumulator_ * inverseMass_;
    velocity_ = velocity_ + acceleration * dt;
    velocity_ = velocity_ * (1.0f - linearDamping_ * dt);

    // === Angular ===
    // World-space inertia tensors from current orientation. (Pose hasn't been
    // advanced yet so this matches pre-step orientation; gyroscopic torque
    // therefore uses pre-step ω against pre-step inertia, consistent with a
    // semi-implicit step.)
    const Matrix3x3 R  = Matrix3x3::FromQuaternion(pose_.orientation);
    const Matrix3x3 Rt = R.Transpose();
    invInertiaWorld_ = R.Multiply(invInertiaLocal_).Multiply(Rt);
    const Matrix3x3 inertiaWorld = R.Multiply(inertiaLocal_).Multiply(Rt);

    const Vector3D angMomentum = inertiaWorld.Multiply(angularVelocity_);
    const Vector3D gyro        = angularVelocity_.CrossProduct(angMomentum) * -1.0f;

    const Vector3D angAccel = invInertiaWorld_.Multiply(torqueAccumulator_ + gyro);
    angularVelocity_ = angularVelocity_ + angAccel * dt;
    angularVelocity_ = angularVelocity_ * (1.0f - angularDamping_ * dt);
}

void RigidBody::IntegratePosition(float dt) {
    if (bodyType_ != BodyType::Dynamic) {
        ClearForces();
        return;
    }
    if (inverseMass_ <= 0.0f) {
        ClearForces();
        return;
    }
    if (sleeping_) {
        ClearForces();
        return;
    }

    pose_.position = pose_.position + velocity_ * dt;

    if (angularVelocity_.Magnitude() > 0.0f) {
        pose_.orientation = pose_.orientation.DeltaRotation(angularVelocity_, dt);
    }

    // Mirror to collider - full pose composition (translation + rotated offset).
    if (collider_) {
        collider_->SetPosition(pose_.position + pose_.orientation.RotateVector(collider_->GetLocalOffset().position));
    }

    ClearForces();
}

void RigidBody::Integrate(float dt, const Vector3D& gravity) {
    IntegrateVelocity(dt, gravity);
    IntegratePosition(dt);
}

} // namespace koilo
