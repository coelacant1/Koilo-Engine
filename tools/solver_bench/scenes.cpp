// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scenes.cpp
 * @brief built-in benchmark scenes.
 *
 * Each scene is a self-contained world setup that the harness runs to a
 * fixed simulated duration. Scenes own their bodies/colliders/joints; the
 * harness reads metrics out of `world` after each Step().
 *
 * Scene catalog:
 *   - stack-N for N in {2, 4, 8, 16}    - vertical box stack on static ground.
 *   - pendulum-chain-8                  - 8-link chain anchored at the top.
 *   - mass-ratio-1-on-1000              - light dynamic on heavy dynamic.
 *   - mass-ratio-1000-on-1              - heavy dynamic on light dynamic.
 *
 * SetCollider seeds pose from the collider's stored position, so we always
 * SetCollider BEFORE SetPose (otherwise pose silently resets to origin).
 */
#include "scene.hpp"

#include <koilo/systems/physics/physicsmaterial.hpp>

namespace koilo {
namespace bench {
namespace {

// ----- helpers ------------------------------------------------------------

} // namespace

// =========================================================================
// Box stack
// =========================================================================
class BoxStackScene : public Scene {
public:
    explicit BoxStackScene(int n, bool disableSleep)
        : n_(n), disableSleep_(disableSleep) {}

    std::string Name() const override {
        return std::string("stack-") + std::to_string(n_) +
               (disableSleep_ ? "-nosleep" : "");
    }

    bool DisableSleep() const override { return disableSleep_; }

    void Build(PhysicsWorld& world) override {
        world.SetGravity(Vector3D(0.0f, -9.81f, 0.0f));

        // Static ground.
        auto ground = std::make_unique<RigidBody>(BodyType::Static, 0.0f);
        auto groundShape = std::make_unique<BoxCollider>(
            Vector3D(0, 0, 0), Vector3D(40, 1, 40));
        groundShape->SetMaterial(PhysicsMaterial(0.8f, 0.0f, 1.0f));
        ground->SetCollider(groundShape.get());
        ground->SetPose(BodyPose(Vector3D(0, -0.5f, 0)));
        world.AddBody(ground.get());
        bodies_.push_back(std::move(ground));
        boxColliders_.push_back(std::move(groundShape));

        const float kGap = 1.10f;  // > box edge to avoid spawn-penetration
        for (int i = 0; i < n_; ++i) {
            auto rb = std::make_unique<RigidBody>(BodyType::Dynamic, 1.0f);
            rb->SetInertiaBox(Vector3D(0.5f, 0.5f, 0.5f));
            rb->SetLinearDamping(0.05f);
            rb->SetAngularDamping(0.05f);

            auto shape = std::make_unique<BoxCollider>(
                Vector3D(0, 0, 0), Vector3D(1, 1, 1));
            shape->SetMaterial(PhysicsMaterial(0.8f, 0.0f, 1.0f));

            rb->SetCollider(shape.get());
            rb->SetPose(BodyPose(Vector3D(0, 0.5f + kGap * i, 0)));

            if (disableSleep_) rb->SetAllowSleep(false);

            world.AddBody(rb.get());
            bodies_.push_back(std::move(rb));
            boxColliders_.push_back(std::move(shape));
        }
    }

private:
    int  n_;
    bool disableSleep_;
};

// =========================================================================
// Pendulum chain - 8 spheres connected by DistanceJoints, anchored at top.
// =========================================================================
class PendulumChainScene : public Scene {
public:
    std::string Name() const override { return "pendulum-chain-8"; }

    float SimDuration() const override { return 4.0f; }

    void Build(PhysicsWorld& world) override {
        world.SetGravity(Vector3D(0.0f, -9.81f, 0.0f));

        // Static anchor at origin (no collider needed for joint-only use).
        auto anchor = std::make_unique<RigidBody>(BodyType::Static, 0.0f);
        anchor->SetPose(BodyPose(Vector3D(0, 5.0f, 0)));
        world.AddBody(anchor.get());
        RigidBody* prev = anchor.get();
        bodies_.push_back(std::move(anchor));

        const int kLinks = 8;
        const float kSeg = 0.4f;
        // Chain hangs initially at +X to displace from rest, so it swings.
        for (int i = 0; i < kLinks; ++i) {
            auto rb = std::make_unique<RigidBody>(BodyType::Dynamic, 0.5f);
            rb->SetInertiaSphere(0.1f);
            rb->SetLinearDamping(0.0f);
            rb->SetAngularDamping(0.0f);

            auto shape = std::make_unique<SphereCollider>(
                Vector3D(0, 0, 0), 0.1f);
            shape->SetMaterial(PhysicsMaterial(0.0f, 0.0f, 1.0f));

            rb->SetCollider(shape.get());
            rb->SetPose(BodyPose(Vector3D((i + 1) * kSeg, 5.0f, 0)));
            world.AddBody(rb.get());

            auto joint = std::make_unique<DistanceJoint>(
                prev, rb.get(),
                Vector3D(0, 0, 0), Vector3D(0, 0, 0), kSeg);
            world.AddJoint(joint.get());

            prev = rb.get();
            bodies_.push_back(std::move(rb));
            sphereColliders_.push_back(std::move(shape));
            joints_.push_back(std::move(joint));
        }
    }
};

// =========================================================================
// Mass-ratio scenes - high mass disparity stresses the solver's effective-
// mass blending. Both directions matter (light-on-heavy is easy, heavy-on-
// light routinely crushes naive PGS).
// =========================================================================
class MassRatioScene : public Scene {
public:
    MassRatioScene(float bottomMass, float topMass, std::string name)
        : bottomMass_(bottomMass), topMass_(topMass), name_(std::move(name)) {}

    std::string Name() const override { return name_; }

    void Build(PhysicsWorld& world) override {
        world.SetGravity(Vector3D(0.0f, -9.81f, 0.0f));

        // Static ground.
        auto ground = std::make_unique<RigidBody>(BodyType::Static, 0.0f);
        auto groundShape = std::make_unique<BoxCollider>(
            Vector3D(0, 0, 0), Vector3D(40, 1, 40));
        groundShape->SetMaterial(PhysicsMaterial(0.8f, 0.0f, 1.0f));
        ground->SetCollider(groundShape.get());
        ground->SetPose(BodyPose(Vector3D(0, -0.5f, 0)));
        world.AddBody(ground.get());
        bodies_.push_back(std::move(ground));
        boxColliders_.push_back(std::move(groundShape));

        // Bottom box.
        auto bottom = std::make_unique<RigidBody>(BodyType::Dynamic, bottomMass_);
        bottom->SetInertiaBox(Vector3D(0.5f, 0.5f, 0.5f));
        bottom->SetLinearDamping(0.05f);
        bottom->SetAngularDamping(0.05f);
        auto bShape = std::make_unique<BoxCollider>(
            Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        bShape->SetMaterial(PhysicsMaterial(0.8f, 0.0f, 1.0f));
        bottom->SetCollider(bShape.get());
        bottom->SetPose(BodyPose(Vector3D(0, 0.5f, 0)));
        world.AddBody(bottom.get());
        bodies_.push_back(std::move(bottom));
        boxColliders_.push_back(std::move(bShape));

        // Top box.
        auto top = std::make_unique<RigidBody>(BodyType::Dynamic, topMass_);
        top->SetInertiaBox(Vector3D(0.5f, 0.5f, 0.5f));
        top->SetLinearDamping(0.05f);
        top->SetAngularDamping(0.05f);
        auto tShape = std::make_unique<BoxCollider>(
            Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        tShape->SetMaterial(PhysicsMaterial(0.8f, 0.0f, 1.0f));
        top->SetCollider(tShape.get());
        top->SetPose(BodyPose(Vector3D(0, 1.6f, 0)));
        world.AddBody(top.get());
        bodies_.push_back(std::move(top));
        boxColliders_.push_back(std::move(tShape));
    }

private:
    float       bottomMass_;
    float       topMass_;
    std::string name_;
};

// =========================================================================
// Registry
// =========================================================================
std::vector<std::unique_ptr<Scene>> RegisterAll() {
    std::vector<std::unique_ptr<Scene>> v;
    v.push_back(std::make_unique<BoxStackScene>(2,  false));
    v.push_back(std::make_unique<BoxStackScene>(4,  false));
    v.push_back(std::make_unique<BoxStackScene>(8,  false));
    v.push_back(std::make_unique<BoxStackScene>(16, false));
    v.push_back(std::make_unique<BoxStackScene>(8,  true)); // sleep-off variant
    v.push_back(std::make_unique<PendulumChainScene>());
    v.push_back(std::make_unique<MassRatioScene>(1000.0f, 1.0f,    "mass-ratio-1000-on-1"));
    v.push_back(std::make_unique<MassRatioScene>(1.0f,    1000.0f, "mass-ratio-1-on-1000"));
    return v;
}

} // namespace bench
} // namespace koilo
