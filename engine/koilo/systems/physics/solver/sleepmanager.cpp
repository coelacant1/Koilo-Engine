// SPDX-License-Identifier: GPL-3.0-or-later
#include "sleepmanager.hpp"

#include "../rigidbody.hpp"

#include <algorithm>
#include <limits>

namespace koilo {

float SleepManager::KineticEnergy(const RigidBody& rb) {
    if (rb.IsStatic()) return 0.0f;
    const float m = rb.GetMass();
    const Vector3D v = rb.GetVelocity();
    const Vector3D w = rb.GetAngularVelocity();
    // Use world-space inertia (= R*I_local*R^T) reconstructed from invInertiaWorld
    // by inverting only the dynamic case: simpler - sum 0.5*m*v^2 + 0.5*ω·I_local·ω
    // (rotation-invariant for diagonal inertia, conservatively over-estimates for
    // off-diagonal cases but is fine for a sleep heuristic).
    const Matrix3x3& I = rb.GetInertiaLocal();
    const Vector3D Iw = I.Multiply(w);
    const float linearKE  = 0.5f * m * v.DotProduct(v);
    const float angularKE = 0.5f * w.DotProduct(Iw);
    return linearKE + angularKE;
}

void SleepManager::WakeIslandsWithMotion(const std::vector<Island>& bridgeIslands,
                                         const std::vector<RigidBody*>& bodies,
                                         const SleepConfig& cfg) {
    for (const Island& isl : bridgeIslands) {
        // Quick scan: is any body in this island moving above the wake threshold?
        bool wakeAll = false;
        for (std::uint32_t bid : isl.bodyIds) {
            if (bid >= bodies.size() || !bodies[bid]) continue;
            RigidBody* rb = bodies[bid];
            if (rb->IsSleeping()) continue;        // sleeping bodies have ~0 KE; ignore.
            if (KineticEnergy(*rb) > cfg.wakeEnergyThreshold) { wakeAll = true; break; }
        }
        if (!wakeAll) continue;
        for (std::uint32_t bid : isl.bodyIds) {
            if (bid >= bodies.size() || !bodies[bid]) continue;
            bodies[bid]->Wake();
        }
    }
}

void SleepManager::AttemptSleep(const std::vector<Island>& islands,
                                const std::vector<RigidBody*>& bodies,
                                float dt,
                                const SleepConfig& cfg) {
    for (const Island& isl : islands) {
        // Skip if any member disallows sleep.
        bool allowSleep = true;
        bool allQuiet = true;
        float minTimer = std::numeric_limits<float>::infinity();

        for (std::uint32_t bid : isl.bodyIds) {
            if (bid >= bodies.size() || !bodies[bid]) continue;
            RigidBody* rb = bodies[bid];
            if (!rb->GetAllowSleep()) { allowSleep = false; break; }
        }
        if (!allowSleep) {
            // Reset timers for the whole island so it doesn't sleep the moment
            // sleep is re-allowed.
            for (std::uint32_t bid : isl.bodyIds) {
                if (bid < bodies.size() && bodies[bid]) bodies[bid]->SetSleepTimer(0.0f);
            }
            continue;
        }

        for (std::uint32_t bid : isl.bodyIds) {
            if (bid >= bodies.size() || !bodies[bid]) continue;
            RigidBody* rb = bodies[bid];
            if (rb->IsSleeping()) continue;        // already asleep - doesn't extend timer
            const float ke = KineticEnergy(*rb);
            if (ke <= cfg.sleepEnergyThreshold) {
                rb->SetSleepTimer(rb->GetSleepTimer() + dt);
            } else {
                rb->SetSleepTimer(0.0f);
                allQuiet = false;
            }
            minTimer = std::min(minTimer, rb->GetSleepTimer());
        }

        if (allQuiet && minTimer >= cfg.sleepTimeRequired) {
            for (std::uint32_t bid : isl.bodyIds) {
                if (bid >= bodies.size() || !bodies[bid]) continue;
                RigidBody* rb = bodies[bid];
                rb->SetVelocityRaw(Vector3D(0, 0, 0));
                rb->SetAngularVelocityRaw(Vector3D(0, 0, 0));
                rb->Sleep();
            }
        }
    }
}

} // namespace koilo
