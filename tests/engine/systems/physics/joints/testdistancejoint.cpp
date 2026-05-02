// SPDX-License-Identifier: GPL-3.0-or-later
#include "testdistancejoint.hpp"

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/joints/distancejoint.hpp>
#include <utils/testhelpers.hpp>

#include <cmath>

using namespace koilo;

namespace {

float Dist(const RigidBody& a, const RigidBody& b) {
    const Vector3D d = a.GetPose().position - b.GetPose().position;
    return std::sqrt(d.X * d.X + d.Y * d.Y + d.Z * d.Z);
}

void StepN(PhysicsWorld& w, int n, float dt) {
    for (int i = 0; i < n; ++i) w.Step(dt);
}

} // namespace

void TestDistanceJoint::TestHoldsAnchorsAtTargetLength() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    RigidBody a(BodyType::Dynamic, 1.0f);
    RigidBody b(BodyType::Dynamic, 1.0f);
    a.SetInertiaSphere(0.5f);
    b.SetInertiaSphere(0.5f);
    a.SetPose(BodyPose(Vector3D(0, 0, 0)));
    b.SetPose(BodyPose(Vector3D(2, 0, 0)));
    a.SetLinearDamping(0.0f);  b.SetLinearDamping(0.0f);
    a.SetAngularDamping(0.0f); b.SetAngularDamping(0.0f);

    w.AddBody(&a);
    w.AddBody(&b);

    // Pull A away - joint should drag B with it, distance preserved.
    a.SetVelocity(Vector3D(-1, 0, 0));

    DistanceJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), 2.0f);
    w.AddJoint(&joint);

    const float dt = 1.0f / 120.0f;
    StepN(w, 240, dt);

    const float d = Dist(a, b);
    UNITY_TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.0f, d, __LINE__, "distance preserved by joint");

    w.RemoveJoint(&joint);
    w.RemoveAllBodies();
}

void TestDistanceJoint::TestPendulumWithStaticAnchor() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, -9.81f, 0));

    RigidBody anchor(BodyType::Static, 0.0f);
    anchor.SetPose(BodyPose(Vector3D(0, 5, 0)));

    RigidBody bob(BodyType::Dynamic, 1.0f);
    bob.SetInertiaSphere(0.25f);
    bob.SetPose(BodyPose(Vector3D(2, 5, 0)));
    bob.SetLinearDamping(0.0f);
    bob.SetAngularDamping(0.0f);

    w.AddBody(&anchor);
    w.AddBody(&bob);

    DistanceJoint joint(&anchor, &bob, Vector3D(0,0,0), Vector3D(0,0,0), 2.0f);
    w.AddJoint(&joint);

    const float dt = 1.0f / 240.0f;
    // Run a half-swing - distance must remain near 2.0 throughout.
    float maxErr = 0.0f;
    for (int i = 0; i < 480; ++i) {
        w.Step(dt);
        const float d = Dist(anchor, bob);
        const float err = std::fabs(d - 2.0f);
        if (err > maxErr) maxErr = err;
    }
    // Soft Baumgarte (beta=0.1) + gravity load -> expect <5% drift.
    UNITY_TEST_ASSERT(maxErr < 0.10f, __LINE__, "pendulum length drift bounded");

    w.RemoveJoint(&joint);
    w.RemoveAllBodies();
}

void TestDistanceJoint::TestJointOnlySceneStillSolves() {
    // No contacts at all - solver must still run for joints.
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    RigidBody a(BodyType::Dynamic, 1.0f);
    RigidBody b(BodyType::Dynamic, 1.0f);
    a.SetInertiaSphere(0.5f);
    b.SetInertiaSphere(0.5f);
    a.SetPose(BodyPose(Vector3D(0, 0, 0)));
    b.SetPose(BodyPose(Vector3D(5, 0, 0)));  // far apart, target=2 -> joint must yank them in
    a.SetLinearDamping(0.0f);  b.SetLinearDamping(0.0f);

    w.AddBody(&a);
    w.AddBody(&b);

    DistanceJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), 2.0f);
    w.AddJoint(&joint);

    StepN(w, 240, 1.0f / 120.0f);
    const float d = Dist(a, b);
    UNITY_TEST_ASSERT(d < 4.5f, __LINE__, "joint-only scene closes gap");

    w.RemoveJoint(&joint);
    w.RemoveAllBodies();
}

void TestDistanceJoint::TestRemoveBodyAutoDetachesJoint() {
    PhysicsWorld w;

    RigidBody a(BodyType::Dynamic, 1.0f);
    RigidBody b(BodyType::Dynamic, 1.0f);
    a.SetInertiaSphere(0.5f);
    b.SetInertiaSphere(0.5f);
    a.SetPose(BodyPose(Vector3D(0, 0, 0)));
    b.SetPose(BodyPose(Vector3D(2, 0, 0)));
    w.AddBody(&a);
    w.AddBody(&b);

    DistanceJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), 2.0f);
    w.AddJoint(&joint);
    UNITY_TEST_ASSERT_EQUAL_INT(1, w.GetJointCount(), __LINE__, "joint registered");

    // Removing one of the joint's bodies must auto-detach the joint to avoid
    // dangling-pointer access during the next step.
    w.RemoveBody(&b);
    UNITY_TEST_ASSERT_EQUAL_INT(0, w.GetJointCount(), __LINE__, "joint auto-detached on body removal");

    // World should still step cleanly.
    w.Step(1.0f / 120.0f);

    w.RemoveAllBodies();
}

void TestDistanceJoint::TestAddJointDeduplicates() {
    PhysicsWorld w;
    RigidBody a(BodyType::Dynamic, 1.0f);
    RigidBody b(BodyType::Dynamic, 1.0f);
    a.SetInertiaSphere(0.5f);
    b.SetInertiaSphere(0.5f);
    w.AddBody(&a); w.AddBody(&b);

    DistanceJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), 1.0f);
    w.AddJoint(&joint);
    w.AddJoint(&joint);
    w.AddJoint(&joint);
    UNITY_TEST_ASSERT_EQUAL_INT(1, w.GetJointCount(), __LINE__, "duplicate AddJoint is no-op");

    w.RemoveJoint(&joint);
    UNITY_TEST_ASSERT_EQUAL_INT(0, w.GetJointCount(), __LINE__, "RemoveJoint clears");

    w.RemoveAllBodies();
}

void TestDistanceJoint::RunAllTests() {
    RUN_TEST(TestHoldsAnchorsAtTargetLength);
    RUN_TEST(TestPendulumWithStaticAnchor);
    RUN_TEST(TestJointOnlySceneStillSolves);
    RUN_TEST(TestRemoveBodyAutoDetachesJoint);
    RUN_TEST(TestAddJointDeduplicates);
}
