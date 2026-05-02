// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file aerodynamicsworld.hpp
 * @brief Owns aerodynamic surface / engine registrations and applies forces
 * per physics substep.
 *
 * Lifecycle: register with a PhysicsWorld via AttachToPhysics(); the
 * AerodynamicsWorld installs a pre-fixed-step callback on the PhysicsWorld
 * that calls Step(fixedDt) at the start of every substep. Detach via
 * DetachFromPhysics() before destroying either world.
 *
 * Storage uses raw `RigidBody*` pointers to live components on the caller
 * side. The caller MUST call UnregisterSurface / UnregisterEngine before
 * the body or surface storage is destroyed; failing to do so results in a
 * dangling pointer dereference inside Step().
 *
 * Wind sampling uses a non-owning `IWindField*` (nullptr = zero wind).
 *
 * Determinism:
 *   - Per-substep callback ordering follows registration order.
 *   - All math is FMA-contraction-safe under -ffp-contract=off (no
 *     wrappers needed; foundation flags handle it).
 *   - std::pow / std::exp / std::atan2 are used and are NOT bit-exact
 *     across libc / architecture - T2 cross-machine bit-exactness is
 *     EXPLICITLY out of scope for this module.
 */

#pragma once

#include "aerodynamicsurface.hpp"
#include "thrustengine.hpp"
#include "windfield.hpp"

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace koilo {
class RigidBody;
class PhysicsWorld;
}

namespace koilo::aero {

class AerodynamicsWorld {
public:
    AerodynamicsWorld();
    ~AerodynamicsWorld();

    AerodynamicsWorld(const AerodynamicsWorld&) = delete;
    AerodynamicsWorld& operator=(const AerodynamicsWorld&) = delete;

    /**
     * @brief Install the per-substep callback on the given PhysicsWorld.
     * Safe to call once. Subsequent calls without DetachFromPhysics()
     * are no-ops.
     */
    void AttachToPhysics(PhysicsWorld* world);

    /**
     * @brief Clear the PhysicsWorld pointer and remove the pre-step callback
     * this AerodynamicsWorld registered. Other systems' pre-step callbacks
     * registered on the same PhysicsWorld are unaffected (keyed
     * unregister).
     */
    void DetachFromPhysics();

    /**
     * @brief Wind field sampler. nullptr = zero wind. Non-owning.
     */
    void SetWindField(IWindField* wind) { wind_ = wind; }
    IWindField* GetWindField() const { return wind_; }

    /**
     * @brief Set the world-space "up" axis used for altitude extraction
     * and ISA atmosphere lookup. Default is +Y. Should be unit-length.
     */
    void SetWorldUp(const Vector3D& up) { worldUp_ = up; }
    const Vector3D& GetWorldUp() const { return worldUp_; }

    /// Numerical handle used to unregister a surface or engine.
    using SurfaceId = std::uint32_t;
    using EngineId  = std::uint32_t;
    static constexpr SurfaceId kInvalidId = 0;

    /**
     * @brief Register an aerodynamic surface attached to `body`. The
     * `surface` storage MUST outlive the registration (we hold a pointer).
     * Returns a non-zero id usable for UnregisterSurface; returns
     * kInvalidId if body or surface is null.
     */
    SurfaceId RegisterSurface(RigidBody* body, AerodynamicSurface* surface);

    /**
     * @brief Register a thrust engine. Same lifetime contract as RegisterSurface.
     */
    EngineId RegisterEngine(RigidBody* body, ThrustEngine* engine);

    /**
     * @brief Remove a surface registration. Safe to call with kInvalidId
     * or unknown ids (no-op).
     */
    void UnregisterSurface(SurfaceId id);

    /**
     * @brief Remove an engine registration.
     */
    void UnregisterEngine(EngineId id);

    /**
     * @brief Drop all surface + engine registrations. Wind field pointer
     * and PhysicsWorld attachment are unaffected.
     */
    void Clear();

    std::size_t GetSurfaceCount() const { return surfaces_.size(); }
    std::size_t GetEngineCount()  const { return engines_.size(); }

    /**
     * @brief Accumulated deterministic sim time [s] (sum of fixed dt for
     * each substep since last Reset). Used to seed wind sampling.
     */
    float GetSimTime() const { return simTime_; }

    /**
     * @brief Reset the deterministic sim time accumulator. Use between
     * replays to ensure wind fields evaluate identically.
     */
    void ResetSimTime() { simTime_ = 0.0f; }

    /**
     * @brief Apply aerodynamic forces for one fixed substep. Called by the
     * PhysicsWorld pre-fixed-step callback (or directly by tests).
     */
    void Step(float fixedDt);

private:
    struct SurfaceEntry {
        SurfaceId           id;
        RigidBody*          body;
        AerodynamicSurface* surface;
    };
    struct EngineEntry {
        EngineId    id;
        RigidBody*  body;
        ThrustEngine* engine;
    };

    void ApplySurface(const SurfaceEntry& entry, float fixedDt);
    void ApplyEngine(const EngineEntry& entry, float fixedDt);

    PhysicsWorld* physics_ = nullptr;
    bool          attached_ = false;
    /// Id returned by PhysicsWorld::RegisterPreFixedStepCallback (typed as
    /// raw uint32_t to avoid pulling physicsworld.hpp into this header). 0
    /// means "no callback registered".
    std::uint32_t preStepCallbackId_ = 0;
    IWindField*   wind_ = nullptr;
    Vector3D      worldUp_{0.0f, 1.0f, 0.0f};
    SurfaceId     nextId_ = 1;
    float         simTime_ = 0.0f;
    std::vector<SurfaceEntry> surfaces_;
    std::vector<EngineEntry>  engines_;

public:
    KL_BEGIN_FIELDS(AerodynamicsWorld)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AerodynamicsWorld)
        KL_METHOD_AUTO(AerodynamicsWorld, AttachToPhysics, "Attach to PhysicsWorld"),
        KL_METHOD_AUTO(AerodynamicsWorld, DetachFromPhysics, "Detach from PhysicsWorld"),
        KL_METHOD_AUTO(AerodynamicsWorld, SetWindField, "Set wind field (IWindField*)"),
        KL_METHOD_AUTO(AerodynamicsWorld, GetWindField, "Get wind field"),
        KL_METHOD_AUTO(AerodynamicsWorld, SetWorldUp, "Set world up axis"),
        KL_METHOD_AUTO(AerodynamicsWorld, GetWorldUp, "Get world up axis"),
        KL_METHOD_AUTO(AerodynamicsWorld, RegisterSurface, "Register an aerodynamic surface"),
        KL_METHOD_AUTO(AerodynamicsWorld, RegisterEngine, "Register a thrust engine"),
        KL_METHOD_AUTO(AerodynamicsWorld, UnregisterSurface, "Unregister a surface"),
        KL_METHOD_AUTO(AerodynamicsWorld, UnregisterEngine, "Unregister an engine"),
        KL_METHOD_AUTO(AerodynamicsWorld, Clear, "Clear all surfaces / engines"),
        KL_METHOD_AUTO(AerodynamicsWorld, GetSurfaceCount, "Get surface count"),
        KL_METHOD_AUTO(AerodynamicsWorld, GetEngineCount, "Get engine count"),
        KL_METHOD_AUTO(AerodynamicsWorld, GetSimTime, "Get accumulated sim time (s)"),
        KL_METHOD_AUTO(AerodynamicsWorld, ResetSimTime, "Reset sim-time accumulator"),
        KL_METHOD_AUTO(AerodynamicsWorld, Step, "Step (one fixed substep)")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AerodynamicsWorld)
        KL_CTOR0(AerodynamicsWorld)
    KL_END_DESCRIBE(AerodynamicsWorld)
};

} // namespace koilo::aero
