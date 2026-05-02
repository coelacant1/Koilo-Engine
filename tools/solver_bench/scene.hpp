// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scene.hpp
 * @brief solver benchmark scene descriptor.
 *
 * A scene is a self-contained PhysicsWorld setup + execution recipe.
 * The harness drives Step() in a loop and records metrics each step.
 * Bodies/colliders are owned by the scene and released on destruction.
 */
#pragma once

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/systems/physics/joints/distancejoint.hpp>

#include <memory>
#include <string>
#include <vector>

namespace koilo {
namespace bench {

/**
 * @brief One canonical benchmark scene. Owns bodies + colliders + joints so
 * pointers in `world` stay valid for the run's lifetime.
 */
class Scene {
public:
    virtual ~Scene() = default;

    /// Stable scene identifier for CSV output.
    virtual std::string Name() const = 0;

    /// Physics dt and total simulated time. Default = 1/120 s, 5 simulated seconds.
    virtual float FixedDt()      const { return 1.0f / 120.0f; }
    virtual float SimDuration()  const { return 5.0f; }

    /// True if the scene wants sleep disabled (set on every body before run).
    virtual bool DisableSleep() const { return false; }

    /// Builds bodies/joints into `world`. World gravity etc. is the scene's responsibility.
    virtual void Build(PhysicsWorld& world) = 0;

protected:
    // Storage so derived scenes can keep ownership of dynamically allocated
    // colliders/bodies/joints without leaking. Pointers stay valid until the
    // scene is destroyed (which happens after the run completes).
    std::vector<std::unique_ptr<RigidBody>>      bodies_;
    std::vector<std::unique_ptr<BoxCollider>>    boxColliders_;
    std::vector<std::unique_ptr<SphereCollider>> sphereColliders_;
    std::vector<std::unique_ptr<CapsuleCollider>> capsuleColliders_;
    std::vector<std::unique_ptr<Joint>>          joints_;
};

/// Make-helpers used by built-in scenes.
std::vector<std::unique_ptr<Scene>> RegisterAll();

} // namespace bench
} // namespace koilo
