// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/debug/debug_visualization.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/collider.hpp>
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <cmath>

namespace koilo {

// CVars controlling debug overlays
static AutoCVar_Bool cvar_physics_debug("physics.debug",
    "Show physics collider wireframes and contact points", false);
static AutoCVar_Bool cvar_r_bounds("r.bounds",
    "Show per-mesh AABB and camera frustum wireframes", false);

// Colors
static const Color kColliderSphere(0.0f, 0.8f, 1.0f, 1.0f);   // Cyan
static const Color kColliderBox(1.0f, 0.8f, 0.0f, 1.0f);       // Yellow
static const Color kColliderCapsule(0.8f, 0.0f, 1.0f, 1.0f);   // Purple
static const Color kContactPoint(1.0f, 0.0f, 0.0f, 1.0f);      // Red
static const Color kContactNormal(1.0f, 1.0f, 0.0f, 1.0f);     // Yellow
static const Color kMeshBounds(0.0f, 1.0f, 0.0f, 0.6f);        // Green
static const Color kFrustum(1.0f, 1.0f, 1.0f, 0.8f);           // White

// -----------------------------------------------------------------------

void DebugVisualization::Submit(PhysicsWorld* physics, Scene* scene,
                                CameraBase* camera) {
    if (cvar_physics_debug.Get() && physics) {
        DrawPhysicsColliders(physics);
        DrawPhysicsContacts(physics);
    }

    if (cvar_r_bounds.Get()) {
        if (scene) DrawSceneBounds(scene);
        if (camera) {
            Vector2D minC = camera->GetCameraMinCoordinate();
            Vector2D maxC = camera->GetCameraMaxCoordinate();
            float vpW = maxC.X - minC.X + 1.0f;
            float vpH = maxC.Y - minC.Y + 1.0f;
            float aspect = (vpH > 0.0f) ? vpW / vpH : 1.0f;
            DrawCameraFrustum(camera, aspect);
        }
    }
}

// -- Physics collider wireframes ----------------------------------------

void DebugVisualization::DrawPhysicsColliders(PhysicsWorld* world) {
    if (!world) return;

    int count = world->GetBodyCount();
    for (int i = 0; i < count; ++i) {
        RigidBody* body = world->GetBody(i);
        if (!body) continue;

        Collider* col = body->GetCollider();
        if (!col || !col->IsEnabled()) continue;

        switch (col->GetType()) {
        case ColliderType::Sphere: {
            auto* sc = static_cast<SphereCollider*>(col);
            DrawWireSphere(sc->GetPosition(), sc->GetRadius(),
                           kColliderSphere);
            break;
        }
        case ColliderType::Box: {
            auto* bc = static_cast<BoxCollider*>(col);
            Vector3D mn = bc->GetMinimum();
            Vector3D mx = bc->GetMaximum();
            DrawWireBox(mn, mx, kColliderBox);
            break;
        }
        case ColliderType::Capsule: {
            auto* cc = static_cast<CapsuleCollider*>(col);
            Vector3D p1, p2;
            cc->GetSegment(p1, p2);
            DrawWireCapsule(p1, p2, cc->GetRadius(), kColliderCapsule);
            break;
        }
        default:
            break;
        }
    }
}

void DebugVisualization::DrawPhysicsContacts(PhysicsWorld* world) {
    if (!world) return;

    const auto& contacts = world->GetDebugContacts();
    for (const auto& c : contacts) {
        float crossSize = 0.15f;
        // Red cross at contact point
        DrawCross(c.contactPoint, crossSize, kContactPoint);

        // Yellow normal line from contact point
        Vector3D normalEnd = c.contactPoint + c.normal.Multiply(2.0f);
        DebugDraw::GetInstance().DrawLine(c.contactPoint, normalEnd,
                                          kContactNormal, 0.0f, true);

        // Penetrating contacts get a larger red marker
        if (c.penetration > 0.01f) {
            DrawCross(c.contactPoint, crossSize * 2.0f, kContactPoint);
        }
    }
}

// -- Scene mesh AABB bounds ---------------------------------------------

void DebugVisualization::DrawSceneBounds(Scene* scene) {
    if (!scene) return;

    unsigned int count = scene->GetMeshCount();
    Mesh** meshes = scene->GetMeshes();
    for (unsigned int i = 0; i < count; ++i) {
        Mesh* mesh = meshes[i];
        if (!mesh || !mesh->IsEnabled()) continue;

        Vector3D mn, mx;
        mesh->GetMinMaxDimensions(mn, mx);

        // Skip degenerate bounds
        Vector3D size = mx - mn;
        if (size.X <= 0.0f && size.Y <= 0.0f && size.Z <= 0.0f) continue;

        DrawWireBox(mn, mx, kMeshBounds);
    }
}

// -- Camera frustum wireframe -------------------------------------------

void DebugVisualization::DrawCameraFrustum(CameraBase* camera,
                                           float aspectRatio) {
    if (!camera) return;
    if (!camera->IsPerspective()) return; // Only perspective for now

    Transform* xform = camera->GetTransform();
    if (!xform) return;

    Vector3D camPos = xform->GetPosition();

    // Build camera orientation from transform rotation + look offset
    xform->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion rot = xform->GetRotation() * camera->GetLookOffset();

    Vector3D forward = rot.RotateVector(Vector3D(0, 0, -1));
    Vector3D up      = rot.RotateVector(Vector3D(0, 1, 0));
    Vector3D right   = rot.RotateVector(Vector3D(1, 0, 0));

    float nearDist = camera->GetNearPlane();
    float farDist  = camera->GetFarPlane();
    float fovRad   = camera->GetFOV() * Mathematics::MPI / 180.0f;

    float tanHalfFov = std::tan(fovRad * 0.5f);
    float nearH = nearDist * tanHalfFov;
    float nearW = nearH * aspectRatio;
    float farH  = farDist * tanHalfFov;
    float farW  = farH * aspectRatio;

    Vector3D nc = camPos + forward.Multiply(nearDist);
    Vector3D fc = camPos + forward.Multiply(farDist);

    // Near plane corners
    Vector3D ntl = nc + up.Multiply(nearH) - right.Multiply(nearW);
    Vector3D ntr = nc + up.Multiply(nearH) + right.Multiply(nearW);
    Vector3D nbl = nc - up.Multiply(nearH) - right.Multiply(nearW);
    Vector3D nbr = nc - up.Multiply(nearH) + right.Multiply(nearW);

    // Far plane corners
    Vector3D ftl = fc + up.Multiply(farH) - right.Multiply(farW);
    Vector3D ftr = fc + up.Multiply(farH) + right.Multiply(farW);
    Vector3D fbl = fc - up.Multiply(farH) - right.Multiply(farW);
    Vector3D fbr = fc - up.Multiply(farH) + right.Multiply(farW);

    auto& dd = DebugDraw::GetInstance();

    // Near plane rectangle
    dd.DrawLine(ntl, ntr, kFrustum, 0.0f, false);
    dd.DrawLine(ntr, nbr, kFrustum, 0.0f, false);
    dd.DrawLine(nbr, nbl, kFrustum, 0.0f, false);
    dd.DrawLine(nbl, ntl, kFrustum, 0.0f, false);

    // Far plane rectangle
    dd.DrawLine(ftl, ftr, kFrustum, 0.0f, false);
    dd.DrawLine(ftr, fbr, kFrustum, 0.0f, false);
    dd.DrawLine(fbr, fbl, kFrustum, 0.0f, false);
    dd.DrawLine(fbl, ftl, kFrustum, 0.0f, false);

    // Connecting edges (near to far)
    dd.DrawLine(ntl, ftl, kFrustum, 0.0f, false);
    dd.DrawLine(ntr, ftr, kFrustum, 0.0f, false);
    dd.DrawLine(nbl, fbl, kFrustum, 0.0f, false);
    dd.DrawLine(nbr, fbr, kFrustum, 0.0f, false);
}

// -- Wireframe primitives -----------------------------------------------

void DebugVisualization::DrawWireBox(const Vector3D& mn, const Vector3D& mx,
                                     const Color& color) {
    auto& dd = DebugDraw::GetInstance();

    // 8 corners
    Vector3D c000(mn.X, mn.Y, mn.Z);
    Vector3D c001(mn.X, mn.Y, mx.Z);
    Vector3D c010(mn.X, mx.Y, mn.Z);
    Vector3D c011(mn.X, mx.Y, mx.Z);
    Vector3D c100(mx.X, mn.Y, mn.Z);
    Vector3D c101(mx.X, mn.Y, mx.Z);
    Vector3D c110(mx.X, mx.Y, mn.Z);
    Vector3D c111(mx.X, mx.Y, mx.Z);

    // 12 edges
    // Bottom face
    dd.DrawLine(c000, c100, color, 0.0f, true);
    dd.DrawLine(c100, c101, color, 0.0f, true);
    dd.DrawLine(c101, c001, color, 0.0f, true);
    dd.DrawLine(c001, c000, color, 0.0f, true);
    // Top face
    dd.DrawLine(c010, c110, color, 0.0f, true);
    dd.DrawLine(c110, c111, color, 0.0f, true);
    dd.DrawLine(c111, c011, color, 0.0f, true);
    dd.DrawLine(c011, c010, color, 0.0f, true);
    // Vertical edges
    dd.DrawLine(c000, c010, color, 0.0f, true);
    dd.DrawLine(c100, c110, color, 0.0f, true);
    dd.DrawLine(c101, c111, color, 0.0f, true);
    dd.DrawLine(c001, c011, color, 0.0f, true);
}

void DebugVisualization::DrawWireSphere(const Vector3D& center, float radius,
                                        const Color& color, int segments) {
    auto& dd = DebugDraw::GetInstance();
    float step = 2.0f * Mathematics::MPI / static_cast<float>(segments);

    // XY ring
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * (i + 1);
        Vector3D p0(center.X + radius * std::cos(a0),
                    center.Y + radius * std::sin(a0), center.Z);
        Vector3D p1(center.X + radius * std::cos(a1),
                    center.Y + radius * std::sin(a1), center.Z);
        dd.DrawLine(p0, p1, color, 0.0f, true);
    }
    // XZ ring
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * (i + 1);
        Vector3D p0(center.X + radius * std::cos(a0), center.Y,
                    center.Z + radius * std::sin(a0));
        Vector3D p1(center.X + radius * std::cos(a1), center.Y,
                    center.Z + radius * std::sin(a1));
        dd.DrawLine(p0, p1, color, 0.0f, true);
    }
    // YZ ring
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * (i + 1);
        Vector3D p0(center.X, center.Y + radius * std::cos(a0),
                    center.Z + radius * std::sin(a0));
        Vector3D p1(center.X, center.Y + radius * std::cos(a1),
                    center.Z + radius * std::sin(a1));
        dd.DrawLine(p0, p1, color, 0.0f, true);
    }
}

void DebugVisualization::DrawWireCapsule(const Vector3D& p1,
                                          const Vector3D& p2,
                                          float radius,
                                          const Color& color,
                                          int segments) {
    auto& dd = DebugDraw::GetInstance();

    // Compute capsule axis and an orthonormal basis
    Vector3D axis = p2 - p1;
    float len = axis.Magnitude();
    if (len < 1e-6f) {
        DrawWireSphere(p1, radius, color, segments);
        return;
    }
    Vector3D axisN = axis.Multiply(1.0f / len);

    // Find a perpendicular vector
    Vector3D perp;
    if (std::abs(axisN.X) < 0.9f)
        perp = Vector3D(1, 0, 0);
    else
        perp = Vector3D(0, 1, 0);

    Vector3D u = axisN.CrossProduct(perp).Normal();
    Vector3D v = axisN.CrossProduct(u);

    float step = 2.0f * Mathematics::MPI / static_cast<float>(segments);

    // Draw two end circles and 4 connecting lines
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * (i + 1);

        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        Vector3D off0 = u.Multiply(radius * c0) + v.Multiply(radius * s0);
        Vector3D off1 = u.Multiply(radius * c1) + v.Multiply(radius * s1);

        // Circle at p1
        dd.DrawLine(p1 + off0, p1 + off1, color, 0.0f, true);
        // Circle at p2
        dd.DrawLine(p2 + off0, p2 + off1, color, 0.0f, true);
    }

    // 4 connecting lines along the length
    for (int i = 0; i < 4; ++i) {
        float a = step * i * (segments / 4);
        Vector3D off = u.Multiply(radius * std::cos(a)) + v.Multiply(radius * std::sin(a));
        dd.DrawLine(p1 + off, p2 + off, color, 0.0f, true);
    }

    // Half-circle arcs at each end (hemisphere wireframe)
    int halfSeg = segments / 2;
    float halfStep = Mathematics::MPI / static_cast<float>(halfSeg);

    // Hemisphere at p1 (pointing away from p2)
    for (int i = 0; i < halfSeg; ++i) {
        float a0 = halfStep * i;
        float a1 = halfStep * (i + 1);

        // Arc in the axis-u plane
        Vector3D q0 = p1 - axisN.Multiply(radius * std::cos(a0)) + u.Multiply(radius * std::sin(a0));
        Vector3D q1 = p1 - axisN.Multiply(radius * std::cos(a1)) + u.Multiply(radius * std::sin(a1));
        dd.DrawLine(q0, q1, color, 0.0f, true);

        // Arc in the axis-v plane
        q0 = p1 - axisN.Multiply(radius * std::cos(a0)) + v.Multiply(radius * std::sin(a0));
        q1 = p1 - axisN.Multiply(radius * std::cos(a1)) + v.Multiply(radius * std::sin(a1));
        dd.DrawLine(q0, q1, color, 0.0f, true);
    }

    // Hemisphere at p2 (pointing away from p1)
    for (int i = 0; i < halfSeg; ++i) {
        float a0 = halfStep * i;
        float a1 = halfStep * (i + 1);

        Vector3D q0 = p2 + axisN.Multiply(radius * std::cos(a0)) + u.Multiply(radius * std::sin(a0));
        Vector3D q1 = p2 + axisN.Multiply(radius * std::cos(a1)) + u.Multiply(radius * std::sin(a1));
        dd.DrawLine(q0, q1, color, 0.0f, true);

        q0 = p2 + axisN.Multiply(radius * std::cos(a0)) + v.Multiply(radius * std::sin(a0));
        q1 = p2 + axisN.Multiply(radius * std::cos(a1)) + v.Multiply(radius * std::sin(a1));
        dd.DrawLine(q0, q1, color, 0.0f, true);
    }
}

void DebugVisualization::DrawCross(const Vector3D& pos, float size,
                                    const Color& color) {
    auto& dd = DebugDraw::GetInstance();
    dd.DrawLine(pos - Vector3D(size, 0, 0), pos + Vector3D(size, 0, 0),
                color, 0.0f, false);
    dd.DrawLine(pos - Vector3D(0, size, 0), pos + Vector3D(0, size, 0),
                color, 0.0f, false);
    dd.DrawLine(pos - Vector3D(0, 0, size), pos + Vector3D(0, 0, size),
                color, 0.0f, false);
}

} // namespace koilo
