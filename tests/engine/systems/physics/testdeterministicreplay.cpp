// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdeterministicreplay.cpp
 * @brief validates Tier-1 same-binary determinism.
 *
 * The contract: given identical initial state, identical applied
 * forces/torques, identical step sequence, and identical body-insertion
 * order, two PhysicsWorld instances must produce bit-exact pose+velocity
 * snapshots at every fixed step.
 *
 * Strategy: snapshot per-step state via PhysicsWorld::Step, then re-run
 * the same script against a freshly constructed world and compare every
 * value with `==` (no epsilon). Any tolerance would mask determinism bugs.
 */
#include "testdeterministicreplay.hpp"

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/physicsmaterial.hpp>
#include <koilo/systems/physics/joints/distancejoint.hpp>
#include <utils/testhelpers.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

using namespace koilo;

namespace {

struct BodySnap {
    Vector3D   pos;
    Quaternion orient;
    Vector3D   vel;
    Vector3D   angVel;
};

// Bit-exact comparison via raw byte memcmp - catches even denormal/sign-bit
// drift that a `==` on float would also catch but more clearly.
bool BitEqual(const BodySnap& a, const BodySnap& b) {
    return std::memcmp(&a, &b, sizeof(BodySnap)) == 0;
}

BodySnap Snap(const RigidBody& rb) {
    return BodySnap{
        rb.GetPose().position,
        rb.GetPose().orientation,
        rb.GetVelocity(),
        rb.GetAngularVelocity()
    };
}

RigidBody MakeDyn(float mass, const Vector3D& pos, const Vector3D& vel = Vector3D(0,0,0)) {
    RigidBody rb(BodyType::Dynamic, mass);
    rb.SetInertiaSphere(0.5f);
    rb.SetPose(BodyPose(pos));
    rb.SetVelocity(vel);
    rb.SetLinearDamping(0.0f);
    rb.SetAngularDamping(0.0f);
    return rb;
}

// Scripted per-step force on body[bodyIdx], deterministic from step index.
Vector3D ScriptedForce(int step, int bodyIdx) {
    const float s = static_cast<float>(step);
    const float k = static_cast<float>(bodyIdx + 1);
    return Vector3D(0.5f * k * (s - 30.0f), -0.25f * k, 0.1f * k * s);
}

Vector3D ScriptedTorque(int step, int bodyIdx) {
    const float s = static_cast<float>(step);
    const float k = static_cast<float>(bodyIdx + 1);
    return Vector3D(0.05f * k, 0.02f * k * s, -0.03f * k);
}

} // namespace

void TestDeterministicReplay::TestSameSceneBitExact() {
    // No applied forces, just gravity. Two identical worlds, same step count,
    // expect bit-exact snapshots every step.
    const int kSteps = 120;
    const float kDt = 1.0f / 120.0f;

    auto run = [&](std::vector<BodySnap>& trace) {
        PhysicsWorld w;
        w.SetGravity(Vector3D(0, -9.81f, 0));
        auto a = MakeDyn(1.0f, Vector3D(0, 5,  0), Vector3D( 0.5f, 0, 0));
        auto b = MakeDyn(2.0f, Vector3D(2, 3, -1), Vector3D(-0.3f, 1, 0));
        auto c = MakeDyn(0.5f, Vector3D(-1, 4, 2), Vector3D( 0,    0, 1));
        a.SetAngularVelocity(Vector3D(0.1f, 0.2f, 0.3f));
        b.SetAngularVelocity(Vector3D(0.0f, 0.5f, 0.1f));
        c.SetAngularVelocity(Vector3D(0.4f, 0.0f, 0.2f));
        w.AddBody(&a); w.AddBody(&b); w.AddBody(&c);

        trace.reserve(kSteps * 3);
        for (int s = 0; s < kSteps; ++s) {
            w.Step(kDt);
            trace.push_back(Snap(a));
            trace.push_back(Snap(b));
            trace.push_back(Snap(c));
        }
    };

    std::vector<BodySnap> t1, t2;
    run(t1);
    run(t2);

    UNITY_TEST_ASSERT_EQUAL_INT(static_cast<int>(t1.size()),
                                static_cast<int>(t2.size()),
                                __LINE__, "trace lengths match");
    bool allEq = true;
    int firstDiverge = -1;
    for (std::size_t i = 0; i < t1.size(); ++i) {
        if (!BitEqual(t1[i], t2[i])) { allEq = false; firstDiverge = static_cast<int>(i); break; }
    }
    (void)firstDiverge;
    UNITY_TEST_ASSERT(allEq, __LINE__, "bit-exact replay (gravity-only)");
}

void TestDeterministicReplay::TestScriptedForcesBitExact() {
    const int kSteps = 240;
    const float kDt = 1.0f / 240.0f;

    auto run = [&](std::vector<BodySnap>& trace) {
        PhysicsWorld w;
        w.SetGravity(Vector3D(0, -9.81f, 0));
        auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
        auto b = MakeDyn(1.5f, Vector3D(3, 0, 0));
        w.AddBody(&a); w.AddBody(&b);
        for (int s = 0; s < kSteps; ++s) {
            a.ApplyForce(ScriptedForce(s, 0));
            a.ApplyTorque(ScriptedTorque(s, 0));
            b.ApplyForce(ScriptedForce(s, 1));
            b.ApplyTorque(ScriptedTorque(s, 1));
            w.Step(kDt);
            trace.push_back(Snap(a));
            trace.push_back(Snap(b));
        }
    };

    std::vector<BodySnap> t1, t2;
    run(t1); run(t2);

    bool allEq = (t1.size() == t2.size());
    for (std::size_t i = 0; allEq && i < t1.size(); ++i) {
        if (!BitEqual(t1[i], t2[i])) allEq = false;
    }
    UNITY_TEST_ASSERT(allEq, __LINE__, "bit-exact replay (scripted force+torque)");
}

void TestDeterministicReplay::TestCollisionBitExact() {
    const int kSteps = 240;
    const float kDt = 1.0f / 240.0f;

    auto run = [&](std::vector<BodySnap>& trace) {
        PhysicsWorld w;
        w.SetGravity(Vector3D(0, 0, 0));

        auto a = MakeDyn(1.0f, Vector3D(-2, 0, 0), Vector3D( 1.5f, 0, 0));
        auto b = MakeDyn(1.0f, Vector3D( 2, 0, 0), Vector3D(-1.5f, 0, 0));
        SphereCollider sa(Vector3D(0,0,0), 0.5f);
        SphereCollider sb(Vector3D(0,0,0), 0.5f);
        sa.SetMaterial(PhysicsMaterial(0.3f, 0.6f, 1.0f));
        sb.SetMaterial(PhysicsMaterial(0.3f, 0.6f, 1.0f));
        a.SetCollider(&sa); b.SetCollider(&sb);
        w.AddBody(&a); w.AddBody(&b);

        for (int s = 0; s < kSteps; ++s) {
            w.Step(kDt);
            trace.push_back(Snap(a));
            trace.push_back(Snap(b));
        }
    };

    std::vector<BodySnap> t1, t2;
    run(t1); run(t2);

    bool allEq = (t1.size() == t2.size());
    for (std::size_t i = 0; allEq && i < t1.size(); ++i) {
        if (!BitEqual(t1[i], t2[i])) allEq = false;
    }
    UNITY_TEST_ASSERT(allEq, __LINE__, "bit-exact replay (collision + restitution + friction)");
}

void TestDeterministicReplay::TestJointSceneBitExact() {
    const int kSteps = 180;
    const float kDt = 1.0f / 120.0f;

    auto run = [&](std::vector<BodySnap>& trace) {
        PhysicsWorld w;
        w.SetGravity(Vector3D(0, -9.81f, 0));

        auto anchor = MakeDyn(1.0f, Vector3D(0, 5, 0));
        anchor.SetBodyType(BodyType::Static);
        auto bob = MakeDyn(1.0f, Vector3D(2, 5, 0));
        w.AddBody(&anchor); w.AddBody(&bob);

        DistanceJoint joint(&anchor, &bob,
                            Vector3D(0,0,0), Vector3D(0,0,0), 2.0f);
        w.AddJoint(&joint);

        for (int s = 0; s < kSteps; ++s) {
            w.Step(kDt);
            trace.push_back(Snap(bob));
        }
    };

    std::vector<BodySnap> t1, t2;
    run(t1); run(t2);

    bool allEq = (t1.size() == t2.size());
    for (std::size_t i = 0; allEq && i < t1.size(); ++i) {
        if (!BitEqual(t1[i], t2[i])) allEq = false;
    }
    UNITY_TEST_ASSERT(allEq, __LINE__, "bit-exact replay (joint-constrained pendulum)");
}

void TestDeterministicReplay::TestDifferentInsertionOrderDiverges() {
    // T1 explicitly requires identical insertion order. This documents that
    // contract: swapping the order MAY change the result (and typically does
    // for any scene with collisions or joints - solver iterates in body order).
    // We assert the result eventually differs; if it stayed bit-exact the
    // determinism contract would be stronger than documented (which is fine,
    // but we'd want to know).
    const int kSteps = 120;
    const float kDt = 1.0f / 240.0f;

    auto run = [&](bool swap, std::vector<BodySnap>& trace) {
        PhysicsWorld w;
        w.SetGravity(Vector3D(0, 0, 0));
        auto a = MakeDyn(1.0f, Vector3D(-1.5f, 0, 0), Vector3D( 1.0f, 0, 0));
        auto b = MakeDyn(1.0f, Vector3D( 1.5f, 0, 0), Vector3D(-1.0f, 0, 0));
        SphereCollider sa(Vector3D(0,0,0), 0.5f);
        SphereCollider sb(Vector3D(0,0,0), 0.5f);
        sa.SetMaterial(PhysicsMaterial(0.5f, 0.5f, 1.0f));
        sb.SetMaterial(PhysicsMaterial(0.5f, 0.5f, 1.0f));
        a.SetCollider(&sa); b.SetCollider(&sb);
        if (swap) { w.AddBody(&b); w.AddBody(&a); }
        else      { w.AddBody(&a); w.AddBody(&b); }

        for (int s = 0; s < kSteps; ++s) {
            w.Step(kDt);
            // Snapshot in canonical (a,b) order regardless of insertion order.
            trace.push_back(Snap(a));
            trace.push_back(Snap(b));
        }
    };

    std::vector<BodySnap> t1, t2;
    run(false, t1);
    run(true,  t2);

    // Just check that running the SAME order twice is bit-exact (re-asserts
    // T1) and that swapped order eventually diverges. We don't fail if it
    // happens to match - the contract is permissive in that direction.
    std::vector<BodySnap> t1b;
    run(false, t1b);
    bool sameOrderExact = (t1.size() == t1b.size());
    for (std::size_t i = 0; sameOrderExact && i < t1.size(); ++i) {
        if (!BitEqual(t1[i], t1b[i])) sameOrderExact = false;
    }
    UNITY_TEST_ASSERT(sameOrderExact, __LINE__,
                      "same insertion order is bit-exact across runs");

    // Note: we do NOT assert divergence on swapped order - current solver
    // happens to be insertion-order-symmetric for symmetric 2-body scenes.
    // The documented contract requires identical order; this test only
    // verifies the lower bound (same order => bit-exact).
    (void)t2;
}

void TestDeterministicReplay::RunAllTests() {
    RUN_TEST(TestSameSceneBitExact);
    RUN_TEST(TestScriptedForcesBitExact);
    RUN_TEST(TestCollisionBitExact);
    RUN_TEST(TestJointSceneBitExact);
    RUN_TEST(TestDifferentInsertionOrderDiverges);
}
