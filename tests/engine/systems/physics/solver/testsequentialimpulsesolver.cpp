// SPDX-License-Identifier: GPL-3.0-or-later
#include "testsequentialimpulsesolver.hpp"

#include <koilo/systems/physics/solver/sequentialimpulsesolver.hpp>
#include <koilo/systems/physics/contactmanifold.hpp>
#include <koilo/systems/physics/contactcache.hpp>
#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <utils/testhelpers.hpp>

#include <vector>
#include <cmath>

using namespace koilo;

namespace {

// Test scaffold: build a dynamic body of given mass, sphere inertia, position.
RigidBody MakeDynamicSphere(float mass, float radius, const Vector3D& pos) {
    RigidBody rb(BodyType::Dynamic, mass);
    BodyPose pose(pos);
    rb.SetPose(pose);
    rb.SetInertiaSphere(radius);
    rb.SetFriction(0.0f);
    rb.SetRestitution(0.0f);
    return rb;
}

ColliderProxy MakeProxy(std::uint32_t bodyId, std::uint32_t proxyId) {
    ColliderProxy p;
    p.bodyId = bodyId;
    p.proxyId = proxyId;
    return p;
}

// Build a single-contact manifold between proxies a and b.
ContactManifold MakeManifold(ColliderProxy* a, ColliderProxy* b,
                             const Vector3D& point, const Vector3D& normal,
                             float depth) {
    ContactManifold m;
    m.a = a;
    m.b = b;
    m.count = 1;
    m.contacts[0].point = point;
    m.contacts[0].normal = normal;
    m.contacts[0].depth = depth;
    m.contacts[0].featureId = 1;
    m.contacts[0].accumulatedNormalImpulse = 0.0f;
    m.contacts[0].accumulatedTangentImpulse[0] = 0.0f;
    m.contacts[0].accumulatedTangentImpulse[1] = 0.0f;
    return m;
}

} // namespace

// Ball moves toward static plane. After solve, vn should be ~0 (no restitution).
void TestSequentialImpulseSolver::TestSingleNormalImpulseDrivesVnToZero() {
    RigidBody ball = MakeDynamicSphere(1.0f, 0.5f, Vector3D(0, 0.5f, 0));
    ball.SetVelocity(Vector3D(0, -2.0f, 0));

    // Static "ground" body.
    RigidBody ground(BodyType::Static, 0.0f); ground.SetRestitution(0.0f); ground.SetFriction(0.0f);
    ground.SetPose(BodyPose(Vector3D(0, 0, 0)));

    ColliderProxy ballProxy = MakeProxy(0, 1);
    ColliderProxy groundProxy = MakeProxy(1, 2);

    // Normal points B->A (ground -> ball), so +Y.
    ContactManifold m = MakeManifold(&ballProxy, &groundProxy,
                                     Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.0f);
    std::vector<ContactManifold> manifolds = { m };
    std::vector<RigidBody*> bodies = { &ball, &ground };
    ContactCache cache;
    cache.Touch(manifolds[0]);   // populate cache entry

    SequentialImpulseSolver solver;
    SolverConfig cfg;
    solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);

    // After solve, the ball's downward velocity should be ~0.
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, ball.GetVelocity().Y);
    // Ground unchanged (static).
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ground.GetVelocity().Y);
    // Accumulator must be positive (impulse along +Y on ball).
    TEST_ASSERT_TRUE(manifolds[0].contacts[0].accumulatedNormalImpulse > 0.0f);
}

// With restitution = 1.0, ball bouncing into wall should reverse normal velocity.
void TestSequentialImpulseSolver::TestRestitutionReversesNormalVelocity() {
    RigidBody ball = MakeDynamicSphere(1.0f, 0.5f, Vector3D(0, 0.5f, 0));
    ball.SetVelocity(Vector3D(0, -3.0f, 0));
    ball.SetRestitution(1.0f);

    RigidBody ground(BodyType::Static, 0.0f); ground.SetRestitution(0.0f); ground.SetFriction(0.0f);
    ground.SetPose(BodyPose(Vector3D(0, 0, 0)));
    ground.SetRestitution(1.0f);

    ColliderProxy bp = MakeProxy(0, 1);
    ColliderProxy gp = MakeProxy(1, 2);
    ContactManifold m = MakeManifold(&bp, &gp, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.0f);
    std::vector<ContactManifold> manifolds = { m };
    std::vector<RigidBody*> bodies = { &ball, &ground };
    ContactCache cache;
    cache.Touch(manifolds[0]);

    SequentialImpulseSolver solver;
    SolverConfig cfg;
    solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);

    // vn0 = -3, e = 1 -> target vn ≈ +3.
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3.0f, ball.GetVelocity().Y);
}

// Sliding box on plane with friction must lose tangential velocity over multiple steps.
void TestSequentialImpulseSolver::TestFrictionStopsTangentialMotion() {
    RigidBody box = MakeDynamicSphere(1.0f, 0.5f, Vector3D(0, 0.5f, 0));
    box.SetVelocity(Vector3D(2.0f, 0.0f, 0.0f));   // sliding +X
    box.SetFriction(1.0f);
    // Disable rotation so friction directly decelerates the COM (otherwise a
    // sphere transitions from slide to roll at v=(5/7)v0 and friction stops doing work).
    box.SetInertiaLocal(Matrix3x3(0, 0, 0, 0, 0, 0, 0, 0, 0));

    RigidBody ground(BodyType::Static, 0.0f); ground.SetRestitution(0.0f); ground.SetFriction(0.0f);
    ground.SetPose(BodyPose(Vector3D(0, 0, 0)));
    ground.SetFriction(1.0f);

    ColliderProxy bp = MakeProxy(0, 1);
    ColliderProxy gp = MakeProxy(1, 2);
    std::vector<RigidBody*> bodies = { &box, &ground };
    ContactCache cache;

    SequentialImpulseSolver solver;
    SolverConfig cfg;

    // Simulate 60 substeps where gravity is implicitly providing the normal load:
    // we model that by giving the ball a downward velocity into the contact each step,
    // which the normal impulse resolves; friction then bites into the +X velocity.
    const float dt = 1.0f / 120.0f;
    const float gravity = 9.81f;
    for (int step = 0; step < 60; ++step) {
        // Integrate "gravity" into the box velocity (semi-implicit).
        Vector3D v = box.GetVelocity();
        v.Y -= gravity * dt;
        box.SetVelocity(v);

        ContactManifold m = MakeManifold(&bp, &gp, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.0f);
        std::vector<ContactManifold> manifolds = { m };
        cache.BeginFrame();
        cache.Touch(manifolds[0]);
        solver.Solve(manifolds, bodies, cache, dt, cfg);
    }

    // With μ=1 and steady normal load, +X velocity must be substantially reduced.
    TEST_ASSERT_TRUE(box.GetVelocity().X < 0.5f);
}

// Two static bodies - solver must not touch their velocities (and must not crash).
void TestSequentialImpulseSolver::TestStaticVsStaticSkipped() {
    RigidBody a(BodyType::Static, 0.0f);
    RigidBody b(BodyType::Static, 0.0f);
    a.SetPose(BodyPose(Vector3D(0, 0.5f, 0)));
    b.SetPose(BodyPose(Vector3D(0, 0, 0)));
    a.SetVelocity(Vector3D(0, -1.0f, 0));   // static bodies "shouldn't" have velocity but force it.

    ColliderProxy ap = MakeProxy(0, 1);
    ColliderProxy bp = MakeProxy(1, 2);
    ContactManifold m = MakeManifold(&ap, &bp, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.1f);
    std::vector<ContactManifold> manifolds = { m };
    std::vector<RigidBody*> bodies = { &a, &b };
    ContactCache cache;
    cache.Touch(manifolds[0]);

    SequentialImpulseSolver solver;
    SolverConfig cfg;
    solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);

    // A's velocity is unchanged because IsStatic() short-circuits the writeback.
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -1.0f, a.GetVelocity().Y);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, b.GetVelocity().Y);
    // Accumulator must still be ~0 (constraint never engaged).
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, manifolds[0].contacts[0].accumulatedNormalImpulse);
}

// Two consecutive solves with cache reuse: the second solve should warm-start
// from the first's accumulator, requiring near-zero additional impulse.
void TestSequentialImpulseSolver::TestWarmStartPreservesAccumulator() {
    RigidBody ball = MakeDynamicSphere(1.0f, 0.5f, Vector3D(0, 0.5f, 0));
    ball.SetVelocity(Vector3D(0, -2.0f, 0));

    RigidBody ground(BodyType::Static, 0.0f); ground.SetRestitution(0.0f); ground.SetFriction(0.0f);
    ground.SetPose(BodyPose(Vector3D(0, 0, 0)));

    ColliderProxy bp = MakeProxy(0, 1);
    ColliderProxy gp = MakeProxy(1, 2);
    std::vector<RigidBody*> bodies = { &ball, &ground };
    ContactCache cache;
    SequentialImpulseSolver solver;
    SolverConfig cfg;

    // First step.
    {
        ContactManifold m = MakeManifold(&bp, &gp, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.0f);
        std::vector<ContactManifold> manifolds = { m };
        cache.BeginFrame();
        cache.Touch(manifolds[0]);
        solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);
    }
    const Vector3D vAfterFirst = ball.GetVelocity();
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, vAfterFirst.Y);

    // Second step (no incoming velocity; cache should warm-start).
    {
        ContactManifold m = MakeManifold(&bp, &gp, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.0f);
        std::vector<ContactManifold> manifolds = { m };
        cache.BeginFrame();
        cache.Touch(manifolds[0]);
        // Warm-start should preload the accumulator from cache.
        TEST_ASSERT_TRUE(manifolds[0].contacts[0].accumulatedNormalImpulse > 0.0f);
        solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);
    }

    // After second solve, ball still at rest.
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, ball.GetVelocity().Y);
}

// A manifold whose two proxies map to the same bodyId must be skipped.
void TestSequentialImpulseSolver::TestSelfContactSkipped() {
    RigidBody a = MakeDynamicSphere(1.0f, 0.5f, Vector3D(0, 0, 0));
    a.SetVelocity(Vector3D(0, -2.0f, 0));

    ColliderProxy p1 = MakeProxy(0, 1);
    ColliderProxy p2 = MakeProxy(0, 2);   // SAME bodyId
    ContactManifold m = MakeManifold(&p1, &p2, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.1f);
    std::vector<ContactManifold> manifolds = { m };
    std::vector<RigidBody*> bodies = { &a };
    ContactCache cache;
    cache.Touch(manifolds[0]);

    SequentialImpulseSolver solver;
    SolverConfig cfg;
    solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);

    // Body's velocity is unchanged.
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -2.0f, a.GetVelocity().Y);
}

// Penetrating ball at rest: Baumgarte bias should impart a positive separation impulse.
void TestSequentialImpulseSolver::TestBaumgartePushesPenetrationApart() {
    RigidBody ball = MakeDynamicSphere(1.0f, 0.5f, Vector3D(0, 0.4f, 0));
    ball.SetVelocity(Vector3D(0, 0, 0));   // at rest, but penetrating.

    RigidBody ground(BodyType::Static, 0.0f); ground.SetRestitution(0.0f); ground.SetFriction(0.0f);
    ground.SetPose(BodyPose(Vector3D(0, 0, 0)));

    ColliderProxy bp = MakeProxy(0, 1);
    ColliderProxy gp = MakeProxy(1, 2);
    // depth = 0.1 (penetrating by 10cm), normal +Y (B->A push).
    ContactManifold m = MakeManifold(&bp, &gp, Vector3D(0, 0, 0), Vector3D(0, 1, 0), 0.1f);
    std::vector<ContactManifold> manifolds = { m };
    std::vector<RigidBody*> bodies = { &ball, &ground };
    ContactCache cache;
    cache.Touch(manifolds[0]);

    SequentialImpulseSolver solver;
    SolverConfig cfg;
    const float yBefore = ball.GetPose().position.Y;
    solver.Solve(manifolds, bodies, cache, 1.0f / 120.0f, cfg);

    // Penetration must be reduced. Under default config (positionIterations>0,
    // 6.5b NGS) this happens via direct pose correction; under positionIterations==0
    // (legacy) it happens via velocity injection. Either way, the ball should be
    // separating from the ground after Solve(): ball moved upward and/or has
    // upward velocity.
    const float yAfter = ball.GetPose().position.Y;
    const float vyAfter = ball.GetVelocity().Y;
    const float separation = (yAfter - yBefore) + vyAfter * (1.0f / 120.0f);
    TEST_ASSERT_TRUE(separation > 0.0f);
}

void TestSequentialImpulseSolver::RunAllTests() {
    RUN_TEST(TestSingleNormalImpulseDrivesVnToZero);
    RUN_TEST(TestRestitutionReversesNormalVelocity);
    RUN_TEST(TestFrictionStopsTangentialMotion);
    RUN_TEST(TestStaticVsStaticSkipped);
    RUN_TEST(TestWarmStartPreservesAccumulator);
    RUN_TEST(TestSelfContactSkipped);
    RUN_TEST(TestBaumgartePushesPenetrationApart);
}
