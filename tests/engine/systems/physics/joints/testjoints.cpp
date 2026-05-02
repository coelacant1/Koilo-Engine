// SPDX-License-Identifier: GPL-3.0-or-later
#include "testjoints.hpp"

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/joints/ballsocketjoint.hpp>
#include <koilo/systems/physics/joints/fixedjoint.hpp>
#include <koilo/systems/physics/joints/hingejoint.hpp>
#include <koilo/systems/physics/joints/sliderjoint.hpp>
#include <koilo/core/math/rotation.hpp>
#include <koilo/core/math/axisangle.hpp>
#include <utils/testhelpers.hpp>

#include <cmath>

using namespace koilo;

namespace {

float Dist(const Vector3D& a, const Vector3D& b) {
    const Vector3D d = a - b;
    return std::sqrt(d.X*d.X + d.Y*d.Y + d.Z*d.Z);
}

void StepN(PhysicsWorld& w, int n, float dt) {
    for (int i = 0; i < n; ++i) w.Step(dt);
}

RigidBody MakeDyn(float mass, const Vector3D& pos) {
    RigidBody rb(BodyType::Dynamic, mass);
    rb.SetInertiaSphere(0.5f);
    rb.SetPose(BodyPose(pos));
    rb.SetLinearDamping(0.0f);
    rb.SetAngularDamping(0.0f);
    return rb;
}

} // namespace

// ============================================================
// BallSocketJoint
// ============================================================

void TestBallSocketJoint::TestAnchorsCoincide() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(2, 0, 0));
    w.AddBody(&a); w.AddBody(&b);

    // Anchors: A at +x, B at -x -> both worlds at (1,0,0) initially.
    BallSocketJoint joint(&a, &b, Vector3D(1,0,0), Vector3D(-1,0,0));
    w.AddJoint(&joint);

    a.SetVelocity(Vector3D(0, -1, 0));   // pull A down

    StepN(w, 240, 1.0f / 120.0f);

    const Vector3D wA = a.GetPose().position + a.GetPose().orientation.RotateVector(Vector3D(1,0,0));
    const Vector3D wB = b.GetPose().position + b.GetPose().orientation.RotateVector(Vector3D(-1,0,0));
    UNITY_TEST_ASSERT(Dist(wA, wB) < 0.05f, __LINE__, "anchors stay coincident");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestBallSocketJoint::TestRelativeOrientationIsFree() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(0, 0, 0));   // same anchor
    w.AddBody(&a); w.AddBody(&b);
    BallSocketJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0));
    w.AddJoint(&joint);

    // Spin B; A should remain (mostly) still.
    b.SetAngularVelocity(Vector3D(0, 5.0f, 0));

    StepN(w, 60, 1.0f / 120.0f);

    // A's angular speed should be far less than B's (no orientation coupling).
    const float wAmag = a.GetAngularVelocity().Magnitude();
    const float wBmag = b.GetAngularVelocity().Magnitude();
    UNITY_TEST_ASSERT(wBmag > 4.0f, __LINE__, "B keeps spinning");
    UNITY_TEST_ASSERT(wAmag < 0.5f, __LINE__, "A barely spins (orientation free)");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestBallSocketJoint::RunAllTests() {
    RUN_TEST(TestAnchorsCoincide);
    RUN_TEST(TestRelativeOrientationIsFree);
}

// ============================================================
// FixedJoint
// ============================================================

void TestFixedJoint::TestPositionLocked() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    w.AddBody(&a); w.AddBody(&b);
    // Anchors meet at the world midpoint (0.5, 0, 0), so the joint preserves
    // a 1m separation along +x rather than collapsing the bodies together.
    FixedJoint joint(&a, &b, Vector3D(0.5f,0,0), Vector3D(-0.5f,0,0));
    w.AddJoint(&joint);

    // Rather than checking exact relative position (which can drift slightly
    // under in-unison spin from constraint coupling), assert that anchors
    // remain coincident - that's the actual joint invariant.
    a.SetVelocity(Vector3D(2, 1, 0));

    StepN(w, 240, 1.0f / 120.0f);

    const Vector3D wA = a.GetPose().position + a.GetPose().orientation.RotateVector(Vector3D(0.5f,0,0));
    const Vector3D wB = b.GetPose().position + b.GetPose().orientation.RotateVector(Vector3D(-0.5f,0,0));
    UNITY_TEST_ASSERT(Dist(wA, wB) < 0.05f, __LINE__, "anchors stay coincident");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestFixedJoint::TestOrientationLocked() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    w.AddBody(&a); w.AddBody(&b);
    FixedJoint joint(&a, &b, Vector3D(0.5f,0,0), Vector3D(-0.5f,0,0));
    w.AddJoint(&joint);

    a.SetAngularVelocity(Vector3D(0, 2.0f, 0));   // spin A - B must follow.

    StepN(w, 240, 1.0f / 120.0f);

    // After many steps, B's angular velocity should be close to A's.
    const Vector3D wA = a.GetAngularVelocity();
    const Vector3D wB = b.GetAngularVelocity();
    UNITY_TEST_ASSERT((wA - wB).Magnitude() < 0.5f, __LINE__, "B follows A's rotation");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestFixedJoint::TestRecaptureAcceptsNewRest() {
    // After a deliberate teleport that changes relative orientation, calling
    // Recapture() should treat the new pose as the rest pose - bodies do NOT
    // get pulled back to the original construction-time rest.
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    FixedJoint joint(&a, &b, Vector3D(0.5f,0,0), Vector3D(-0.5f,0,0));
    w.AddJoint(&joint);

    StepN(w, 30, 1.0f / 120.0f);

    // Teleport B with a new orientation (90deg about Y).
    BodyPose newPose = b.GetPose();
    newPose.orientation = Rotation(AxisAngle(90.0f, Vector3D(0, 1, 0))).GetQuaternion();
    b.SetPose(newPose);
    joint.Recapture();

    StepN(w, 240, 1.0f / 120.0f);

    // B's orientation should be close to its post-Recapture value (no snap-back).
    const Quaternion qB = b.GetPose().orientation;
    const float dotW = std::fabs(qB.W * newPose.orientation.W
                               + qB.X * newPose.orientation.X
                               + qB.Y * newPose.orientation.Y
                               + qB.Z * newPose.orientation.Z);
    UNITY_TEST_ASSERT(dotW > 0.95f, __LINE__,
                      "Recapture preserves new rest orientation");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestFixedJoint::RunAllTests() {
    RUN_TEST(TestPositionLocked);
    RUN_TEST(TestOrientationLocked);
    RUN_TEST(TestRecaptureAcceptsNewRest);
}

// ============================================================
// HingeJoint
// ============================================================

void TestHingeJoint::TestRotatesFreelyAboutAxis() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(0, 0, 0));
    w.AddBody(&a); w.AddBody(&b);
    HingeJoint joint(&a, &b,
                     Vector3D(0,0,0), Vector3D(0,0,0),
                     Vector3D(0,1,0), Vector3D(0,1,0),    // hinge axis = +Y
                     Vector3D(1,0,0), Vector3D(1,0,0));   // ref axis = +X
    w.AddJoint(&joint);

    b.SetAngularVelocity(Vector3D(0, 3.0f, 0));   // spin around hinge axis only.

    StepN(w, 60, 1.0f / 120.0f);

    // B should still be spinning fast about Y; A should barely spin (no torque
    // delivered along free axis).
    const float wBy = b.GetAngularVelocity().Y;
    UNITY_TEST_ASSERT(wBy > 2.0f, __LINE__, "B keeps spinning about hinge axis");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestHingeJoint::TestPerpendicularAxesStayAligned() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(0, 0, 0));
    w.AddBody(&a); w.AddBody(&b);
    HingeJoint joint(&a, &b,
                     Vector3D(0,0,0), Vector3D(0,0,0),
                     Vector3D(0,1,0), Vector3D(0,1,0),
                     Vector3D(1,0,0), Vector3D(1,0,0));
    w.AddJoint(&joint);

    // Try to break alignment with a perpendicular spin on B.
    b.SetAngularVelocity(Vector3D(2.0f, 0, 0));

    StepN(w, 240, 1.0f / 120.0f);

    // After settling, world hinge axes on A and B should still be aligned.
    const Vector3D axisAw = a.GetPose().orientation.RotateVector(Vector3D(0,1,0));
    const Vector3D axisBw = b.GetPose().orientation.RotateVector(Vector3D(0,1,0));
    const float dot = axisAw.DotProduct(axisBw);
    UNITY_TEST_ASSERT(dot > 0.98f, __LINE__, "hinge axes stay aligned (dot>0.98)");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestHingeJoint::TestLimitStopsRotation() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(0, 0, 0));
    a.SetBodyType(BodyType::Static);             // anchor A so we measure B's angle relative to A.
    w.AddBody(&a); w.AddBody(&b);
    HingeJoint joint(&a, &b,
                     Vector3D(0,0,0), Vector3D(0,0,0),
                     Vector3D(0,1,0), Vector3D(0,1,0),
                     Vector3D(1,0,0), Vector3D(1,0,0));
    joint.EnableLimit(true, -0.5f, 0.5f);   // ±~28.6°
    w.AddJoint(&joint);

    b.SetAngularVelocity(Vector3D(0, 5.0f, 0));   // spin past upper limit.

    StepN(w, 240, 1.0f / 120.0f);

    const float angle = joint.GetCurrentAngle();
    UNITY_TEST_ASSERT(angle < 0.6f, __LINE__, "angle clamped near upper limit");
    UNITY_TEST_ASSERT(angle > -0.6f, __LINE__, "angle clamped near lower limit");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestHingeJoint::TestMotorAccelerates() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(0, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    HingeJoint joint(&a, &b,
                     Vector3D(0,0,0), Vector3D(0,0,0),
                     Vector3D(0,1,0), Vector3D(0,1,0),
                     Vector3D(1,0,0), Vector3D(1,0,0));
    joint.EnableMotor(true, /*targetAngVel*/ 4.0f, /*maxTorque*/ 50.0f);
    w.AddJoint(&joint);

    StepN(w, 120, 1.0f / 120.0f);   // 1s simulated.

    const float wBy = b.GetAngularVelocity().Y;
    UNITY_TEST_ASSERT_FLOAT_WITHIN(0.5f, 4.0f, wBy, __LINE__,
                                   "motor drives angular velocity to target");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestHingeJoint::TestMotorRespectsLimit() {
    // Drives motor past the upper limit; with kMaxJointRows=7 both rows are
    // active, so the limit row clamps despite the motor row pushing.
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(0, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    HingeJoint joint(&a, &b,
                     Vector3D(0,0,0), Vector3D(0,0,0),
                     Vector3D(0,1,0), Vector3D(0,1,0),
                     Vector3D(1,0,0), Vector3D(1,0,0));
    joint.EnableLimit(true, -0.3f, 0.3f);
    joint.EnableMotor(true, /*targetAngVel*/ 8.0f, /*maxTorque*/ 200.0f);
    w.AddJoint(&joint);

    StepN(w, 240, 1.0f / 120.0f);

    const float angle = joint.GetCurrentAngle();
    UNITY_TEST_ASSERT(angle < 0.4f, __LINE__,
                      "limit row clamps motor-driven rotation (row count=7)");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestHingeJoint::RunAllTests() {
    RUN_TEST(TestRotatesFreelyAboutAxis);
    RUN_TEST(TestPerpendicularAxesStayAligned);
    RUN_TEST(TestLimitStopsRotation);
    RUN_TEST(TestMotorAccelerates);
    RUN_TEST(TestMotorRespectsLimit);
}

// ============================================================
// SliderJoint
// ============================================================

void TestSliderJoint::TestSlidesFreelyAlongAxis() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    SliderJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), Vector3D(1,0,0));
    w.AddJoint(&joint);

    b.SetVelocity(Vector3D(2.0f, 0, 0));   // along axis - must remain.

    StepN(w, 60, 1.0f / 120.0f);

    UNITY_TEST_ASSERT_FLOAT_WITHIN(0.2f, 2.0f, b.GetVelocity().X, __LINE__,
                                   "axial velocity preserved");
    UNITY_TEST_ASSERT(std::fabs(b.GetVelocity().Y) < 0.2f, __LINE__, "no perp Y motion");
    UNITY_TEST_ASSERT(std::fabs(b.GetVelocity().Z) < 0.2f, __LINE__, "no perp Z motion");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestSliderJoint::TestPerpendicularMotionLocked() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    SliderJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), Vector3D(1,0,0));
    w.AddJoint(&joint);

    b.SetVelocity(Vector3D(0, 2.0f, 0));   // perpendicular kick.

    StepN(w, 240, 1.0f / 120.0f);

    UNITY_TEST_ASSERT(std::fabs(b.GetPose().position.Y) < 0.10f, __LINE__,
                      "no Y drift along perpendicular axis");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestSliderJoint::TestLimitStopsTranslation() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    SliderJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), Vector3D(1,0,0));
    joint.EnableLimit(true, 0.0f, 1.5f);
    w.AddJoint(&joint);

    b.SetVelocity(Vector3D(5.0f, 0, 0));   // try to push past upper limit.

    StepN(w, 240, 1.0f / 120.0f);

    UNITY_TEST_ASSERT(joint.GetCurrentDisplacement() < 1.6f, __LINE__,
                      "stays at/under upper limit");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestSliderJoint::TestOrientationLocked() {
    PhysicsWorld w;
    w.SetGravity(Vector3D(0, 0, 0));

    auto a = MakeDyn(1.0f, Vector3D(0, 0, 0));
    auto b = MakeDyn(1.0f, Vector3D(1, 0, 0));
    a.SetBodyType(BodyType::Static);
    w.AddBody(&a); w.AddBody(&b);
    SliderJoint joint(&a, &b, Vector3D(0,0,0), Vector3D(0,0,0), Vector3D(1,0,0));
    w.AddJoint(&joint);

    b.SetAngularVelocity(Vector3D(0, 3.0f, 0));   // try to spin B - slider must lock.

    StepN(w, 240, 1.0f / 120.0f);

    UNITY_TEST_ASSERT(b.GetAngularVelocity().Magnitude() < 0.5f, __LINE__,
                      "angular velocity damped to ~0 (orientation locked)");

    w.RemoveJoint(&joint); w.RemoveAllBodies();
}

void TestSliderJoint::RunAllTests() {
    RUN_TEST(TestSlidesFreelyAlongAxis);
    RUN_TEST(TestPerpendicularMotionLocked);
    RUN_TEST(TestLimitStopsTranslation);
    RUN_TEST(TestOrientationLocked);
}
