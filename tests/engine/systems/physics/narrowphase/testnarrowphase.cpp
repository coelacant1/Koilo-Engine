// SPDX-License-Identifier: GPL-3.0-or-later
#include "testnarrowphase.hpp"
#include <koilo/systems/physics/narrowphase/gjk.hpp>
#include <koilo/systems/physics/narrowphase/epa.hpp>
#include <koilo/systems/physics/narrowphase/manifoldgenerator.hpp>
#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/shape/sphereshape.hpp>
#include <koilo/systems/physics/shape/boxshape.hpp>
#include <koilo/systems/physics/shape/capsuleshape.hpp>
#include <koilo/systems/physics/shape/planeshape.hpp>
#include <koilo/systems/physics/shape/trianglemeshshape.hpp>
#include <koilo/systems/physics/shape/heightfieldshape.hpp>
#include <koilo/systems/physics/shape/heightfielddata.hpp>
#include <utils/testhelpers.hpp>
#include <cmath>
#include <cstdio>

using namespace koilo;

namespace {
inline float Dot(const Vector3D& a, const Vector3D& b) { return a.X*b.X + a.Y*b.Y + a.Z*b.Z; }
}

// ---------------- GJK ----------------

void TestGjk::TestSeparatedSpheres() {
    SphereShape s(1.0f);
    auto r = Gjk(s, BodyPose(Vector3D(0,0,0)), s, BodyPose(Vector3D(5,0,0)));
    TEST_ASSERT_FALSE(r.intersect);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.0f, r.distance);
}

void TestGjk::TestTouchingSpheres() {
    SphereShape s(1.0f);
    auto r = Gjk(s, BodyPose(Vector3D(0,0,0)), s, BodyPose(Vector3D(2.0f,0,0)));
    // Touching is treated as intersect (origin on the boundary of the Minkowski difference).
    TEST_ASSERT_TRUE(r.intersect || r.distance < 1e-3f);
}

void TestGjk::TestPenetratingSpheres() {
    SphereShape s(1.0f);
    auto r = Gjk(s, BodyPose(Vector3D(0,0,0)), s, BodyPose(Vector3D(1.0f,0,0)));
    TEST_ASSERT_TRUE(r.intersect);
}

void TestGjk::TestSeparatedBoxes() {
    BoxShape bx(Vector3D(0.5f,0.5f,0.5f));
    auto r = Gjk(bx, BodyPose(Vector3D(0,0,0)), bx, BodyPose(Vector3D(3.0f,0,0)));
    TEST_ASSERT_FALSE(r.intersect);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.0f, r.distance);
}

void TestGjk::TestPenetratingBoxes() {
    BoxShape bx(Vector3D(0.5f,0.5f,0.5f));
    auto r = Gjk(bx, BodyPose(Vector3D(0,0,0)), bx, BodyPose(Vector3D(0.5f,0,0)));
    TEST_ASSERT_TRUE(r.intersect);
}

void TestGjk::RunAllTests() {
    RUN_TEST(TestSeparatedSpheres);
    RUN_TEST(TestTouchingSpheres);
    RUN_TEST(TestPenetratingSpheres);
    RUN_TEST(TestSeparatedBoxes);
    RUN_TEST(TestPenetratingBoxes);
}

// ---------------- EPA ----------------

void TestEpa::TestSpherePenetrationDepthAndNormal() {
    SphereShape s(1.0f);
    BodyPose pa(Vector3D(0,0,0));
    BodyPose pb(Vector3D(1.5f,0,0));
    auto g = Gjk(s, pa, s, pb);
    TEST_ASSERT_TRUE(g.intersect);
    auto e = Epa(s, pa, s, pb, g);
    TEST_ASSERT_TRUE(e.ok);
    // Expected depth = 2*r - dist = 2 - 1.5 = 0.5
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, e.depth);
    // Normal points from B toward A => -X direction.
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -1.0f, e.normal.X);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, e.normal.Y);
}

void TestEpa::TestBoxPenetrationOnAxis() {
    BoxShape bx(Vector3D(0.5f,0.5f,0.5f));
    BodyPose pa(Vector3D(0,0,0));
    BodyPose pb(Vector3D(0.7f,0,0));   // overlap of 0.3 along X
    auto g = Gjk(bx, pa, bx, pb);
    TEST_ASSERT_TRUE(g.intersect);
    auto e = Epa(bx, pa, bx, pb, g);
    TEST_ASSERT_TRUE(e.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.3f, e.depth);
    TEST_ASSERT_TRUE(std::fabs(e.normal.X) > 0.9f);
}

void TestEpa::RunAllTests() {
    RUN_TEST(TestSpherePenetrationDepthAndNormal);
    RUN_TEST(TestBoxPenetrationOnAxis);
}

// ---------------- Manifold Generator ----------------

namespace {
struct MGFixture {
    SphereShape sphere{1.0f};
    BoxShape    box{Vector3D(0.5f,0.5f,0.5f)};
    PlaneShape  plane{Vector3D(0,1,0), 0.0f};
    ColliderProxy a, b;
    MGFixture() {
        a.proxyId = 0;
        b.proxyId = 1;
    }
};
}

void TestManifoldGenerator::TestSphereSphereContact() {
    MGFixture f;
    f.a.shape = &f.sphere;
    f.b.shape = &f.sphere;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0,0,0)),
                                           &f.b, BodyPose(Vector3D(1.5f,0,0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_EQUAL_UINT8(1, m.count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, m.contacts[0].depth);
}

void TestManifoldGenerator::TestSphereSphereSeparation() {
    MGFixture f;
    f.a.shape = &f.sphere;
    f.b.shape = &f.sphere;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0,0,0)),
                                           &f.b, BodyPose(Vector3D(5,0,0)), m);
    TEST_ASSERT_FALSE(hit);
    TEST_ASSERT_EQUAL_UINT8(0, m.count);
}

void TestManifoldGenerator::TestSpherePlaneContact() {
    MGFixture f;
    f.a.shape = &f.sphere;
    f.b.shape = &f.plane;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0,0.5f,0)),
                                           &f.b, BodyPose(Vector3D(0,0,0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_EQUAL_UINT8(1, m.count);
    // Normal points from plane (B) toward sphere (A): +Y.
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, m.contacts[0].normal.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, m.contacts[0].depth);
}

void TestManifoldGenerator::TestPlaneSphereContact() {
    MGFixture f;
    // Pass plane first; manifold must canonicalize so sphere ends up first (proxyId 0).
    f.a.shape = &f.plane;   // proxyId 0
    f.b.shape = &f.sphere;  // proxyId 1
    ContactManifold m;
    // Need sphere as proxyId 0. Re-rig:
    f.a.shape = &f.sphere;
    f.b.shape = &f.plane;
    bool hit = ManifoldGenerator::Generate(&f.b, BodyPose(Vector3D(0,0,0)),  // plane (id 1)
                                           &f.a, BodyPose(Vector3D(0,0.5f,0)), // sphere (id 0)
                                           m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_EQUAL_PTR(&f.a, m.a);  // sphere is canonical first
    TEST_ASSERT_EQUAL_PTR(&f.b, m.b);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestSphereBoxContact() {
    MGFixture f;
    f.a.shape = &f.sphere;
    f.b.shape = &f.box;
    ContactManifold m;
    // Sphere center 1.2 from box center along X; sphere r=1, box he=0.5 -> overlap 0.3.
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(1.2f,0,0)),
                                           &f.b, BodyPose(Vector3D(0,0,0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.3f, m.contacts[0].depth);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, m.contacts[0].normal.X);
}

void TestManifoldGenerator::TestBoxBoxPenetration() {
    MGFixture f;
    f.a.shape = &f.box;
    f.b.shape = &f.box;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0,0,0)),
                                           &f.b, BodyPose(Vector3D(0.7f,0,0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_TRUE(m.contacts[0].depth > 0.0f);
}

void TestManifoldGenerator::TestBoxBoxStackedFourContacts() {
    // Two unit-half-extent boxes face-aligned, slightly overlapping along Y.
    // Top box sits on bottom box with 0.1 penetration. Expect 4 face contacts.
    MGFixture f;
    f.a.shape = &f.box;   // half = 0.5
    f.b.shape = &f.box;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0, 0.0f, 0)),
                                           &f.b, BodyPose(Vector3D(0, 0.9f, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_EQUAL_UINT8(4, m.count);
    for (int i = 0; i < m.count; ++i) {
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.1f, m.contacts[i].depth);
        // Normal points B (top) -> A (bottom), so along -Y.
        TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.0f, m.contacts[i].normal.Y);
    }
}

void TestManifoldGenerator::TestBoxBoxEdgeEdgeSingleContact() {
    // Box A axis-aligned at origin; box B rotated 45° about Z then 45° about X
    // and offset along the diagonal so an edge-edge axis wins SAT.
    MGFixture f;
    f.a.shape = &f.box;
    f.b.shape = &f.box;
    // 45° about Z then 45° about X. Quaternion (w, x, y, z) for axis-angle:
    // q = (cos(θ/2), axis * sin(θ/2)). For 45°, cos(22.5°) ≈ 0.92388, sin(22.5°) ≈ 0.38268.
    const float c = 0.9238795325f, s = 0.3826834324f;
    Quaternion qz(c, 0, 0, s);
    Quaternion qx(c, s, 0, 0);
    Quaternion q = qz * qx;
    BodyPose pb(Vector3D(0.85f, 0.85f, 0.0f), q);
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0,0,0)), &f.b, pb, m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_TRUE(m.contacts[0].depth > 0.0f);
}

void TestManifoldGenerator::TestNormalConventionFromBToA() {
    // For sphere-sphere, A at origin, B at +X. Normal must be -X.
    MGFixture f;
    f.a.shape = &f.sphere;
    f.b.shape = &f.sphere;
    ContactManifold m;
    ManifoldGenerator::Generate(&f.a, BodyPose(Vector3D(0,0,0)),
                                &f.b, BodyPose(Vector3D(1.5f,0,0)), m);
    TEST_ASSERT_TRUE(m.contacts[0].normal.X < -0.9f);
}

void TestManifoldGenerator::TestProxyIdCanonicalization() {
    MGFixture f;
    f.a.shape = &f.sphere;
    f.b.shape = &f.sphere;
    ContactManifold m;
    // Pass in reverse order; manifold should still come out with a->proxyId < b->proxyId.
    ManifoldGenerator::Generate(&f.b, BodyPose(Vector3D(1.5f,0,0)),
                                &f.a, BodyPose(Vector3D(0,0,0)), m);
    TEST_ASSERT_EQUAL_PTR(&f.a, m.a);
    TEST_ASSERT_EQUAL_PTR(&f.b, m.b);
    TEST_ASSERT_TRUE(m.a->proxyId < m.b->proxyId);
}

#include <koilo/systems/physics/shape/trianglemeshshape.hpp>
#include <koilo/systems/physics/shape/trianglemeshdata.hpp>
#include <memory>

namespace {

// Builds a flat 2-triangle quad in the XZ plane at y=0, spanning [-2,2] x [-2,2],
// with a CCW winding so the +Y direction is the "outside" face normal.
std::shared_ptr<koilo::TriangleMeshData> BuildQuadGround() {
    auto data = std::make_shared<koilo::TriangleMeshData>();
    data->vertices = {
        koilo::Vector3D(-2,0,-2),
        koilo::Vector3D( 2,0,-2),
        koilo::Vector3D( 2,0, 2),
        koilo::Vector3D(-2,0, 2)
    };
    data->indices = { 0,1,2,  0,2,3 };
    data->Build();
    return data;
}

} // namespace

void TestManifoldGenerator::TestSphereTriangleMeshFace() {
    using namespace koilo;
    auto data = BuildQuadGround();
    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    // Sphere centered at (0, 0.5, 0): center is 0.5 above the face, sphere r=1, depth=0.5.
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(0, 0.5f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.5f, m.contacts[0].depth);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestSphereTriangleMeshSeparated() {
    using namespace koilo;
    auto data = BuildQuadGround();
    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    // Sphere well above the ground.
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(0, 5.0f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_FALSE(hit);
    TEST_ASSERT_EQUAL_UINT8(0, m.count);
}

void TestManifoldGenerator::TestSphereTriangleMeshEdge() {
    using namespace koilo;
    auto data = BuildQuadGround();
    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    // Sphere center sits past the +X edge: closest point is on the edge x=2.
    // Place center at (2.5, 0.5, 0): closest = (2,0,0), dist=sqrt(.25+.25)=~0.707, depth ~ 0.293.
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(2.5f, 0.5f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.293f, m.contacts[0].depth);
    // Normal should point from edge toward sphere center -> roughly (+x, +y) normalized.
    const float nlen = std::sqrt(m.contacts[0].normal.X * m.contacts[0].normal.X +
                                 m.contacts[0].normal.Y * m.contacts[0].normal.Y +
                                 m.contacts[0].normal.Z * m.contacts[0].normal.Z);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, nlen);
    TEST_ASSERT_TRUE(m.contacts[0].normal.X > 0.0f);
    TEST_ASSERT_TRUE(m.contacts[0].normal.Y > 0.0f);
}

void TestManifoldGenerator::TestSphereTriangleMeshVertex() {
    using namespace koilo;
    auto data = BuildQuadGround();
    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    // Sphere center past the (+2,0,+2) vertex: closest is the vertex itself.
    // Place at (2.5, 0.5, 2.5): dist = sqrt(.25+.25+.25) ~ 0.866, depth ~ 0.134.
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(2.5f, 0.5f, 2.5f)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.134f, m.contacts[0].depth);
    TEST_ASSERT_TRUE(m.contacts[0].normal.X > 0.0f);
    TEST_ASSERT_TRUE(m.contacts[0].normal.Y > 0.0f);
    TEST_ASSERT_TRUE(m.contacts[0].normal.Z > 0.0f);
}

void TestManifoldGenerator::TestMeshSphereReverseDispatch() {
    using namespace koilo;
    auto data = BuildQuadGround();
    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    // Mesh proxyId smaller -> dispatched as (Mesh, Sphere); generator must canonicalize.
    ColliderProxy pm; pm.proxyId = 0; pm.shape = &mesh;
    ColliderProxy ps; ps.proxyId = 1; ps.shape = &sphere;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pm, BodyPose(Vector3D(0, 0, 0)),
                                           &ps, BodyPose(Vector3D(0, 0.5f, 0)), m);
    TEST_ASSERT_TRUE(hit);
    // Canonical: a should be the mesh (proxyId 0), b the sphere (proxyId 1).
    TEST_ASSERT_EQUAL_PTR(&pm, m.a);
    TEST_ASSERT_EQUAL_PTR(&ps, m.b);
    // Normal still points from B (sphere) toward A (mesh): -Y.
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -1.0f, m.contacts[0].normal.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.5f, m.contacts[0].depth);
}

void TestManifoldGenerator::TestSphereTriangleMeshRotated() {
    using namespace koilo;
    auto data = BuildQuadGround();
    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    // Rotate the mesh 90deg around +Z: ground (originally XZ plane) becomes
    // the YZ plane; +Y face normal rotates to +X. Sphere placed at (0.5, 0, 0)
    // should contact the wall with normal ~ +X and depth 0.5.
    Quaternion q;
    q.W = std::cos(M_PI * 0.25f);
    q.X = 0.0f; q.Y = 0.0f;
    q.Z = std::sin(M_PI * 0.25f);
    BodyPose meshPose(Vector3D(0, 0, 0), q);
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(0.5f, 0, 0)),
                                           &pm, meshPose, m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, m.contacts[0].depth);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, m.contacts[0].normal.X);
}

void TestManifoldGenerator::TestSphereTriangleMeshDegenerateIgnored() {
    using namespace koilo;
    // Mesh with one good triangle and one degenerate (collinear) triangle.
    auto data = std::make_shared<TriangleMeshData>();
    data->vertices = {
        Vector3D(-1, 0, -1), Vector3D(1, 0, -1), Vector3D(1, 0, 1),
        Vector3D(0, 0, 0),   Vector3D(1, 0, 0),  Vector3D(2, 0, 0)   // collinear -> degenerate
    };
    data->indices = { 0,1,2,  3,4,5 };
    data->Build();
    // Degenerate filter should leave 1 triangle in the BVH.
    TEST_ASSERT_EQUAL_UINT32(1u, data->bvh.TriangleCount());

    SphereShape sphere(1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(0, 0.5f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
}

void TestManifoldGenerator::TestCapsuleTriangleMeshFace() {
    using namespace koilo;
    auto data = BuildQuadGround();
    CapsuleShape cap(0.5f, 1.0f); // r=0.5, hh=1.0 -> segment along ±Y
    TriangleMeshShape mesh(data);
    ColliderProxy pc; pc.proxyId = 0; pc.shape = &cap;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    // Capsule centred at (0, 0.7, 0): bottom hemisphere extends to y=-0.8 -> 0.8 below ground.
    // Wait: segment endpoints at y=±1.0 in local; with r=0.5, capsule extends from y=-1.5 to +1.5.
    // Centre at y=0.7 -> bottom = -0.8. Depth = 0.8 (penetrates by 0.8).
    bool hit = ManifoldGenerator::Generate(&pc, BodyPose(Vector3D(0, 0.7f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_TRUE(m.contacts[0].depth > 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestCapsuleTriangleMeshSeparated() {
    using namespace koilo;
    auto data = BuildQuadGround();
    CapsuleShape cap(0.5f, 1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy pc; pc.proxyId = 0; pc.shape = &cap;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pc, BodyPose(Vector3D(0, 5.0f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_FALSE(hit);
    TEST_ASSERT_EQUAL_UINT8(0, m.count);
}

void TestManifoldGenerator::TestCapsuleTriangleMeshParallelOverInterior() {
    using namespace koilo;
    auto data = BuildQuadGround();
    // Capsule rotated 90° around +Z so segment runs along world +X (parallel to ground plane).
    CapsuleShape cap(0.3f, 1.5f);
    TriangleMeshShape mesh(data);
    Quaternion q;
    q.W = std::cos(static_cast<float>(M_PI) / 4.0f);
    q.Z = std::sin(static_cast<float>(M_PI) / 4.0f);
    BodyPose poseCap(Vector3D(0, 0.2f, 0), q); // capsule axis world-X, hovering 0.2 above ground
    ColliderProxy pc; pc.proxyId = 0; pc.shape = &cap;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pc, poseCap,
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    // Both endpoints hover 0.2 above ground (well within radius 0.3) -> must collide.
    // Tests the "perpendicular foot in triangle" candidate path.
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.1f, m.contacts[0].depth); // 0.3 - 0.2 = 0.1
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestMeshCapsuleReverseDispatch() {
    using namespace koilo;
    auto data = BuildQuadGround();
    CapsuleShape cap(0.5f, 1.0f);
    TriangleMeshShape mesh(data);
    ColliderProxy pm; pm.proxyId = 0; pm.shape = &mesh;
    ColliderProxy pc; pc.proxyId = 1; pc.shape = &cap;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pm, BodyPose(Vector3D(0, 0, 0)),
                                           &pc, BodyPose(Vector3D(0, 0.7f, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    // Canonical order: mesh proxyId=0 < capsule proxyId=1, so a=mesh, b=capsule.
    // Normal points from B (capsule) toward A (mesh) -> -Y for capsule above ground.
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestBoxTriangleMeshFaceMultiContact() {
    using namespace koilo;
    auto data = BuildQuadGround();
    BoxShape box(Vector3D(1.0f, 0.5f, 1.0f)); // 2x1x2 box
    TriangleMeshShape mesh(data);
    ColliderProxy pb; pb.proxyId = 0; pb.shape = &box;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    // Box centred at (0, 0.3, 0): bottom face at y=-0.2 -> penetrates 0.2 into ground.
    // Box face is 2x2, ground quad is 4x4 -> all 4 box bottom verts project inside the quad.
    bool hit = ManifoldGenerator::Generate(&pb, BodyPose(Vector3D(0, 0.3f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    // Multi-contact: must have ≥3 contacts for stable resting on the quad.
    TEST_ASSERT_TRUE(m.count >= 3);
    for (std::uint8_t i = 0; i < m.count; ++i) {
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.2f, m.contacts[i].depth);
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, m.contacts[i].normal.Y);
    }
}

void TestManifoldGenerator::TestBoxTriangleMeshSeparated() {
    using namespace koilo;
    auto data = BuildQuadGround();
    BoxShape box(Vector3D(0.5f, 0.5f, 0.5f));
    TriangleMeshShape mesh(data);
    ColliderProxy pb; pb.proxyId = 0; pb.shape = &box;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pb, BodyPose(Vector3D(0, 5.0f, 0)),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_FALSE(hit);
    TEST_ASSERT_EQUAL_UINT8(0, m.count);
}

void TestManifoldGenerator::TestBoxTriangleMeshEdge() {
    using namespace koilo;
    auto data = BuildQuadGround();
    BoxShape box(Vector3D(0.5f, 0.5f, 0.5f));
    TriangleMeshShape mesh(data);
    // Box rotated 45° around +Z so a single edge points down.
    Quaternion q;
    q.W = std::cos(static_cast<float>(M_PI) / 8.0f);
    q.Z = std::sin(static_cast<float>(M_PI) / 8.0f);
    // Box centred so its lowest edge is at y ≈ -0.1 (penetrates 0.1).
    // Half-diagonal of 0.5×0.5 face = sqrt(0.5) ≈ 0.707; place centre at y=0.6 -> low ≈ -0.1.
    ColliderProxy pb; pb.proxyId = 0; pb.shape = &box;
    ColliderProxy pm; pm.proxyId = 1; pm.shape = &mesh;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pb, BodyPose(Vector3D(0, 0.6f, 0), q),
                                           &pm, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_TRUE(m.contacts[0].normal.Y > 0.5f);
}

void TestManifoldGenerator::TestMeshBoxReverseDispatch() {
    using namespace koilo;
    auto data = BuildQuadGround();
    BoxShape box(Vector3D(1.0f, 0.5f, 1.0f));
    TriangleMeshShape mesh(data);
    ColliderProxy pm; pm.proxyId = 0; pm.shape = &mesh;
    ColliderProxy pb; pb.proxyId = 1; pb.shape = &box;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pm, BodyPose(Vector3D(0, 0, 0)),
                                           &pb, BodyPose(Vector3D(0, 0.3f, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    // a=mesh, b=box -> normal from B(box) toward A(mesh) -> -Y.
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.0f, m.contacts[0].normal.Y);
}

namespace {

// Builds a 4x4 cell flat heightfield at y=0 (cellSize=1, so spans [-2,2] in X/Z).
std::shared_ptr<koilo::HeightfieldData> BuildFlatHeightfield() {
    auto data = std::make_shared<koilo::HeightfieldData>();
    data->widthCells = 4;
    data->depthCells = 4;
    data->cellSize   = 1.0f;
    data->heights.assign(5 * 5, 0.0f);
    data->Build();
    return data;
}

} // namespace

void TestManifoldGenerator::TestSphereHeightfieldFlat() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    SphereShape sphere(1.0f);
    HeightfieldShape hf(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy ph; ph.proxyId = 1; ph.shape = &hf;
    ContactManifold m;
    // Sphere center 0.5 above the flat surface -> depth 0.5.
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(0, 0.5f, 0)),
                                           &ph, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, m.contacts[0].depth);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestSphereHeightfieldSeparated() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    SphereShape sphere(1.0f);
    HeightfieldShape hf(data);
    ColliderProxy ps; ps.proxyId = 0; ps.shape = &sphere;
    ColliderProxy ph; ph.proxyId = 1; ph.shape = &hf;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&ps, BodyPose(Vector3D(0, 5.0f, 0)),
                                           &ph, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_FALSE(hit);
    TEST_ASSERT_EQUAL_UINT8(0, m.count);
}

void TestManifoldGenerator::TestSphereHeightfieldReverseDispatch() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    SphereShape sphere(1.0f);
    HeightfieldShape hf(data);
    ColliderProxy ph; ph.proxyId = 0; ph.shape = &hf;
    ColliderProxy ps; ps.proxyId = 1; ps.shape = &sphere;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&ph, BodyPose(Vector3D(0, 0, 0)),
                                           &ps, BodyPose(Vector3D(0, 0.5f, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    // a=heightfield, b=sphere -> normal from B(sphere) toward A(hf) -> -Y.
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::TestCapsuleHeightfieldFlat() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    CapsuleShape cap(0.5f, 2.0f);
    HeightfieldShape hf(data);
    ColliderProxy pc; pc.proxyId = 0; pc.shape = &cap;
    ColliderProxy ph; ph.proxyId = 1; ph.shape = &hf;
    ContactManifold m;
    // Capsule axis is local Y; place center at y=0.8 -> bottom hemisphere center
    // at y=-0.2 with radius 0.5 -> reaches y=-0.7 -> depth ~0.7 vs flat ground.
    bool hit = ManifoldGenerator::Generate(&pc, BodyPose(Vector3D(0, 0.8f, 0)),
                                           &ph, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    TEST_ASSERT_TRUE(m.contacts[0].normal.Y > 0.5f);
}

void TestManifoldGenerator::TestCapsuleHeightfieldSeparated() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    CapsuleShape cap(0.5f, 2.0f);
    HeightfieldShape hf(data);
    ColliderProxy pc; pc.proxyId = 0; pc.shape = &cap;
    ColliderProxy ph; ph.proxyId = 1; ph.shape = &hf;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pc, BodyPose(Vector3D(0, 5.0f, 0)),
                                           &ph, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_FALSE(hit);
}

void TestManifoldGenerator::TestBoxHeightfieldFlatMultiContact() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    BoxShape box(Vector3D(1.0f, 0.5f, 1.0f));
    HeightfieldShape hf(data);
    ColliderProxy pb; pb.proxyId = 0; pb.shape = &box;
    ColliderProxy ph; ph.proxyId = 1; ph.shape = &hf;
    ContactManifold m;
    // Box centered at y=0.3 -> bottom face at y=-0.2 (sunk 0.2 into flat ground).
    bool hit = ManifoldGenerator::Generate(&pb, BodyPose(Vector3D(0, 0.3f, 0)),
                                           &ph, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 3);
    for (std::uint8_t i = 0; i < m.count; ++i) {
        TEST_ASSERT_TRUE(m.contacts[i].normal.Y > 0.5f);
    }
}

void TestManifoldGenerator::TestBoxHeightfieldSeparated() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    BoxShape box(Vector3D(0.5f, 0.5f, 0.5f));
    HeightfieldShape hf(data);
    ColliderProxy pb; pb.proxyId = 0; pb.shape = &box;
    ColliderProxy ph; ph.proxyId = 1; ph.shape = &hf;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&pb, BodyPose(Vector3D(0, 5.0f, 0)),
                                           &ph, BodyPose(Vector3D(0, 0, 0)), m);
    TEST_ASSERT_FALSE(hit);
}

void TestManifoldGenerator::TestBoxHeightfieldReverseDispatch() {
    using namespace koilo;
    auto data = BuildFlatHeightfield();
    BoxShape box(Vector3D(1.0f, 0.5f, 1.0f));
    HeightfieldShape hf(data);
    ColliderProxy ph; ph.proxyId = 0; ph.shape = &hf;
    ColliderProxy pb; pb.proxyId = 1; pb.shape = &box;
    ContactManifold m;
    bool hit = ManifoldGenerator::Generate(&ph, BodyPose(Vector3D(0, 0, 0)),
                                           &pb, BodyPose(Vector3D(0, 0.3f, 0)), m);
    TEST_ASSERT_TRUE(hit);
    TEST_ASSERT_TRUE(m.count >= 1);
    // a=heightfield, b=box -> normal from B(box) toward A(hf) -> -Y.
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.0f, m.contacts[0].normal.Y);
}

void TestManifoldGenerator::RunAllTests() {
    RUN_TEST(TestSphereSphereContact);
    RUN_TEST(TestSphereSphereSeparation);
    RUN_TEST(TestSpherePlaneContact);
    RUN_TEST(TestPlaneSphereContact);
    RUN_TEST(TestSphereBoxContact);
    RUN_TEST(TestBoxBoxPenetration);
    RUN_TEST(TestBoxBoxStackedFourContacts);
    RUN_TEST(TestBoxBoxEdgeEdgeSingleContact);
    RUN_TEST(TestNormalConventionFromBToA);
    RUN_TEST(TestProxyIdCanonicalization);
    RUN_TEST(TestSphereTriangleMeshFace);
    RUN_TEST(TestSphereTriangleMeshSeparated);
    RUN_TEST(TestSphereTriangleMeshEdge);
    RUN_TEST(TestSphereTriangleMeshVertex);
    RUN_TEST(TestMeshSphereReverseDispatch);
    RUN_TEST(TestSphereTriangleMeshRotated);
    RUN_TEST(TestSphereTriangleMeshDegenerateIgnored);
    RUN_TEST(TestCapsuleTriangleMeshFace);
    RUN_TEST(TestCapsuleTriangleMeshSeparated);
    RUN_TEST(TestCapsuleTriangleMeshParallelOverInterior);
    RUN_TEST(TestMeshCapsuleReverseDispatch);
    RUN_TEST(TestBoxTriangleMeshFaceMultiContact);
    RUN_TEST(TestBoxTriangleMeshSeparated);
    RUN_TEST(TestBoxTriangleMeshEdge);
    RUN_TEST(TestMeshBoxReverseDispatch);
    RUN_TEST(TestSphereHeightfieldFlat);
    RUN_TEST(TestSphereHeightfieldSeparated);
    RUN_TEST(TestSphereHeightfieldReverseDispatch);
    RUN_TEST(TestCapsuleHeightfieldFlat);
    RUN_TEST(TestCapsuleHeightfieldSeparated);
    RUN_TEST(TestBoxHeightfieldFlatMultiContact);
    RUN_TEST(TestBoxHeightfieldSeparated);
    RUN_TEST(TestBoxHeightfieldReverseDispatch);
}
