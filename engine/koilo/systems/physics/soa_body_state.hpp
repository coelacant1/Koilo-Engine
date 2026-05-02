// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file soa_body_state.hpp
 * @brief Structure-of-Arrays shadow buffer for rigid-body solver state.
 *
 * SoA layout commitment. RigidBody public API stays AoS for ergonomics;
 * the solver iterates this SoA mirror so SIMD / parallel-island
 * work doesn't require a refactor later. Index stability across add/remove
 * via free-list - handles match RigidBody::physicsHandle.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/matrix3x3.hpp>

#include <vector>
#include <cstdint>
#include <cstddef>

namespace koilo {

/**
 * @class SoaBodyState
 * @brief Parallel arrays of body state fields, indexed by stable handle.
 *
 * No solver math here - pure storage. The solver reads/writes this directly.
 */
class SoaBodyState {
public:
    using Handle = std::uint32_t;
    static constexpr Handle kInvalid = static_cast<Handle>(-1);

    /** Per-body flag bits. */
    enum Flag : std::uint32_t {
        FlagNone     = 0,
        FlagAsleep   = 1u << 0,
        FlagStatic   = 1u << 1,
        FlagKinematic= 1u << 2,
        FlagBullet   = 1u << 3
    };

    Handle Add(const Vector3D& position,
               const Quaternion& orientation,
               float invMass,
               const Matrix3x3& invInertiaWorld,
               std::uint32_t flags = FlagNone) {
        Handle h;
        if (!freeList_.empty()) {
            h = freeList_.back();
            freeList_.pop_back();
            positions_[h]      = position;
            orientations_[h]   = orientation;
            linearVel_[h]      = Vector3D(0,0,0);
            angularVel_[h]     = Vector3D(0,0,0);
            invMass_[h]        = invMass;
            invInertiaWorld_[h]= invInertiaWorld;
            flags_[h]          = flags;
            alive_[h]          = 1;
        } else {
            h = static_cast<Handle>(positions_.size());
            positions_.push_back(position);
            orientations_.push_back(orientation);
            linearVel_.emplace_back(0,0,0);
            angularVel_.emplace_back(0,0,0);
            invMass_.push_back(invMass);
            invInertiaWorld_.push_back(invInertiaWorld);
            flags_.push_back(flags);
            alive_.push_back(1);
        }
        ++liveCount_;
        return h;
    }

    void Remove(Handle h) {
        if (h >= alive_.size() || !alive_[h]) return;
        alive_[h] = 0;
        freeList_.push_back(h);
        --liveCount_;
    }

    bool IsAlive(Handle h) const { return h < alive_.size() && alive_[h] != 0; }
    std::size_t Capacity() const { return positions_.size(); }
    std::size_t Size() const { return liveCount_; }

    void Clear() {
        positions_.clear(); orientations_.clear();
        linearVel_.clear(); angularVel_.clear();
        invMass_.clear(); invInertiaWorld_.clear();
        flags_.clear(); alive_.clear(); freeList_.clear();
        liveCount_ = 0;
    }

    // ---- Accessors (mutable for solver, const for queries) ------------------
    Vector3D&       Position(Handle h)        { return positions_[h]; }
    const Vector3D& Position(Handle h) const  { return positions_[h]; }
    Quaternion&       Orientation(Handle h)       { return orientations_[h]; }
    const Quaternion& Orientation(Handle h) const { return orientations_[h]; }
    Vector3D&       LinearVelocity(Handle h)        { return linearVel_[h]; }
    const Vector3D& LinearVelocity(Handle h) const  { return linearVel_[h]; }
    Vector3D&       AngularVelocity(Handle h)       { return angularVel_[h]; }
    const Vector3D& AngularVelocity(Handle h) const { return angularVel_[h]; }
    float&       InverseMass(Handle h)       { return invMass_[h]; }
    float        InverseMass(Handle h) const { return invMass_[h]; }
    Matrix3x3&       InverseInertiaWorld(Handle h)       { return invInertiaWorld_[h]; }
    const Matrix3x3& InverseInertiaWorld(Handle h) const { return invInertiaWorld_[h]; }
    std::uint32_t&   Flags(Handle h)       { return flags_[h]; }
    std::uint32_t    Flags(Handle h) const { return flags_[h]; }

    // Raw array access (for SIMD/parallel iteration).
    Vector3D*   PositionsData()       { return positions_.data(); }
    Quaternion* OrientationsData()    { return orientations_.data(); }
    Vector3D*   LinearVelData()       { return linearVel_.data(); }
    Vector3D*   AngularVelData()      { return angularVel_.data(); }
    float*      InvMassData()         { return invMass_.data(); }
    Matrix3x3*  InvInertiaWorldData() { return invInertiaWorld_.data(); }
    std::uint32_t* FlagsData()        { return flags_.data(); }
    const std::uint8_t* AliveData() const { return alive_.data(); }

private:
    std::vector<Vector3D>     positions_;
    std::vector<Quaternion>   orientations_;
    std::vector<Vector3D>     linearVel_;
    std::vector<Vector3D>     angularVel_;
    std::vector<float>        invMass_;
    std::vector<Matrix3x3>    invInertiaWorld_;
    std::vector<std::uint32_t> flags_;
    std::vector<std::uint8_t> alive_;
    std::vector<Handle>       freeList_;
    std::size_t               liveCount_ = 0;
};

} // namespace koilo
