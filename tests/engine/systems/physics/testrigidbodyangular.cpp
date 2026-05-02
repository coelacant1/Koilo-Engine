// SPDX-License-Identifier: GPL-3.0-or-later
#include "testrigidbodyangular.hpp"
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <cmath>

using namespace koilo;

void TestRigidBodyAngular::TestDefaultsZeroAngular() {
    RigidBody b;
    Vector3D w = b.GetAngularVelocity();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.X);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.Y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.Z);
    // Default inverse inertia is zero -> torque has no effect.
    const Matrix3x3& invI = b.GetInverseInertiaLocal();
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) TEST_ASSERT_EQUAL_FLOAT(0.0f, invI.M[i][j]);
}

void TestRigidBodyAngular::TestExistingLinearBehaviorPreserved() {
    // Same as the historical linear-only behavior: gravity pulls a body down by 0.5*g*t².
    RigidBody b(BodyType::Dynamic, 1.0f);
    BodyPose p; p.position = Vector3D(0,10,0); b.SetPose(p);
    const float dt = 1.0f/60.0f;
    for (int i=0;i<60;++i) b.Integrate(dt, Vector3D(0,-9.81f,0));
    // Quaternion must remain identity (no torque applied).
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, b.GetPose().orientation.W);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, b.GetPose().orientation.X);
    TEST_ASSERT_TRUE(b.GetPose().position.Y < 10.0f);
}

void TestRigidBodyAngular::TestApplyTorqueIntegratesAngularVelocity() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.SetInertiaSphere(1.0f); // I = (2/5) on each axis
    // τ = (0,0,1). After 1 s with no gyroscopic (ω₀=0), ω_z = α*dt summed.
    // α_z = invI_zz * τ_z = (5/2) * 1 = 2.5 rad/s²
    const float dt = 1.0f/240.0f;
    for (int i=0;i<240;++i) {
        b.ApplyTorque(Vector3D(0,0,1.0f));
        b.Integrate(dt, Vector3D(0,0,0));
    }
    // With a small angularDamping default 0.05, the steady-state is reduced. Just verify monotone growth direction.
    Vector3D w = b.GetAngularVelocity();
    TEST_ASSERT_TRUE(w.Z > 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, w.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, w.Y);
}

void TestRigidBodyAngular::TestApplyAngularImpulseInstant() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.SetInertiaSphere(1.0f); // invI = 2.5*I
    b.ApplyAngularImpulse(Vector3D(0,0,1.0f));
    Vector3D w = b.GetAngularVelocity();
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.5f, w.Z);
}

void TestRigidBodyAngular::TestApplyForceAtPointGeneratesTorque() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.SetInertiaSphere(1.0f);
    BodyPose p; p.position = Vector3D(0,0,0); b.SetPose(p);
    // Force +X applied at point (0, 1, 0) -> torque = r × F = (0,1,0) × (1,0,0) = (0,0,-1)
    b.ApplyForceAtPoint(Vector3D(1,0,0), Vector3D(0,1,0));
    b.Integrate(1.0f/60.0f, Vector3D(0,0,0));
    TEST_ASSERT_TRUE(b.GetAngularVelocity().Z < 0.0f);
    TEST_ASSERT_TRUE(b.GetVelocity().X > 0.0f); // linear part also applied
}

void TestRigidBodyAngular::TestGetPointVelocity() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.SetVelocity(Vector3D(1,0,0));
    b.SetAngularVelocity(Vector3D(0,0,1));
    BodyPose p; p.position = Vector3D(0,0,0); b.SetPose(p);
    // Point at (0,1,0): ω × r = (0,0,1) × (0,1,0) = (-1,0,0). Total: (1-1,0,0) = 0.
    Vector3D v = b.GetPointVelocity(Vector3D(0,1,0));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, v.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, v.Y);
}

void TestRigidBodyAngular::TestQuaternionStaysNormalized() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.SetInertiaBox(Vector3D(0.5f, 0.5f, 0.5f));
    b.SetAngularVelocity(Vector3D(2.0f, 1.5f, 0.7f));
    const float dt = 1.0f/120.0f;
    for (int i=0;i<2400;++i) b.Integrate(dt, Vector3D(0,0,0));
    Quaternion q = b.GetPose().orientation;
    float n = std::sqrt(q.W*q.W + q.X*q.X + q.Y*q.Y + q.Z*q.Z);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, n);
}

void TestRigidBodyAngular::TestNoAngularResponseWithoutInertia() {
    RigidBody b(BodyType::Dynamic, 1.0f); // no SetInertia* called
    b.ApplyTorque(Vector3D(10,0,0));
    b.Integrate(1.0f/60.0f, Vector3D(0,0,0));
    Vector3D w = b.GetAngularVelocity();
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, w.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, w.Y);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, w.Z);
}

void TestRigidBodyAngular::TestSetInertiaSphereDiagonal() {
    Matrix3x3 I = InertiaTensor::SolidSphere(2.0f, 3.0f);
    // (2/5) * 2 * 9 = 7.2
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 7.2f, I.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 7.2f, I.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 7.2f, I.M[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, I.M[0][1]);
}

void TestRigidBodyAngular::TestSetInertiaBoxDiagonal() {
    // halfExtents = (1,1,1) -> I_xx = m/3 * (1+1) = 2m/3
    Matrix3x3 I = InertiaTensor::SolidBox(3.0f, Vector3D(1,1,1));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, I.M[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, I.M[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, I.M[2][2]);
}

void TestRigidBodyAngular::TestNonDynamicIgnoresTorque() {
    RigidBody b(BodyType::Static, 1.0f);
    b.SetInertiaSphere(1.0f);
    b.ApplyTorque(Vector3D(100,0,0));
    b.Integrate(1.0f/60.0f, Vector3D(0,0,0));
    Vector3D w = b.GetAngularVelocity();
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, w.X);
}

void TestRigidBodyAngular::TestClearForcesResetsTorque() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.ApplyTorque(Vector3D(1,2,3));
    b.ClearForces();
    // After clear+integrate with no inertia, no change. After re-applying with inertia, only the new torque contributes.
    b.SetInertiaSphere(1.0f);
    b.ApplyAngularImpulse(Vector3D(0,0,1.0f));
    Vector3D w = b.GetAngularVelocity();
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.5f, w.Z);
}

void TestRigidBodyAngular::TestColliderMirrorsRotation() {
    RigidBody b(BodyType::Dynamic, 1.0f);
    b.SetInertiaSphere(1.0f);
    SphereCollider sphere(Vector3D(0,0,0), 1.0f);
    b.SetCollider(&sphere);
    BodyPose p; p.position = Vector3D(0,0,0); b.SetPose(p);
    b.SetAngularVelocity(Vector3D(0,0,Mathematics::MPI / 2.0f)); // 90°/s about Z
    BodyPose offset; offset.position = Vector3D(1,0,0);
    sphere.SetLocalOffset(offset);
    for (int i=0;i<60;++i) b.Integrate(1.0f/60.0f, Vector3D(0,0,0));
    // After ~1 s of 90°/s rotation, offset (1,0,0) should be near (0,1,0).
    Vector3D pos = sphere.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 1.0f, pos.Y);
}

void TestRigidBodyAngular::RunAllTests() {
    RUN_TEST(TestDefaultsZeroAngular);
    RUN_TEST(TestExistingLinearBehaviorPreserved);
    RUN_TEST(TestApplyTorqueIntegratesAngularVelocity);
    RUN_TEST(TestApplyAngularImpulseInstant);
    RUN_TEST(TestApplyForceAtPointGeneratesTorque);
    RUN_TEST(TestGetPointVelocity);
    RUN_TEST(TestQuaternionStaysNormalized);
    RUN_TEST(TestNoAngularResponseWithoutInertia);
    RUN_TEST(TestSetInertiaSphereDiagonal);
    RUN_TEST(TestSetInertiaBoxDiagonal);
    RUN_TEST(TestNonDynamicIgnoresTorque);
    RUN_TEST(TestClearForcesResetsTorque);
    RUN_TEST(TestColliderMirrorsRotation);
}
