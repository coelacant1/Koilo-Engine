// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/debug/debugrenderer.hpp>

#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/collider.hpp>
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/systems/render/raster/rasterizer.hpp>

namespace koilo {

// --- High-level API ----------------------------------------

void DebugRenderer::Render(DebugDraw& dd, CameraBase* camera) {
    if (!camera || !dd.IsEnabled()) return;

    IPixelGroup* pg = camera->GetPixelGroup();
    if (!pg) return;

    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    int w = static_cast<int>(maxC.X - minC.X + 1);
    int h = static_cast<int>(maxC.Y - minC.Y + 1);

    Color888* buf = pg->GetColorBuffer();
    if (!buf || w <= 0 || h <= 0) return;

    RenderInto(dd, camera, buf, w, h, minC, maxC);
}

void DebugRenderer::Render(DebugDraw& dd, CameraBase* camera,
                           Color888* buffer, int w, int h,
                           const float* depthBuf,
                           int depthW, int depthH,
                           bool renderLines) {
    if (!camera || !dd.IsEnabled() || !buffer || w <= 0 || h <= 0) return;

    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    RenderInto(dd, camera, buffer, w, h, minC, maxC, depthBuf, depthW, depthH, renderLines);
}

// --- Private: RenderInto -----------------------------------

void DebugRenderer::RenderInto(DebugDraw& dd, CameraBase* camera,
                               Color888* buffer, int w, int h,
                               const Vector2D& minC, const Vector2D& maxC,
                               const float* depthBuf,
                               int depthW, int depthH,
                               bool renderLines) {

    // Projection params
    bool persp = camera->IsPerspective();
    float fovScale = 0.0f;
    float nearPlane = camera->GetNearPlane();
    if (persp) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        fovScale = 1.0f / Mathematics::Tan(fovRad * 0.5f);
    }

    camera->GetTransform()->SetBaseRotation(
        camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation()
                            .Multiply(camera->GetLookOffset());
    Quaternion invRot = lookDir.Conjugate();
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D camScale = camera->GetTransform()->GetScale();

    // --- Render lines (skipped when Rasterizer::DrawLine3D handles them) ---
    if (renderLines) {
        for (const auto& line : dd.GetLines()) {
            int x0, y0, x1, y1;
            float z0, z1;
            if (ProjectLineClipped(line.start, line.end,
                                   camPos, invRot, camScale,
                                   persp, fovScale, nearPlane,
                                   minC, maxC, w, h,
                                   x0, y0, x1, y1, z0, z1)) {
                if (depthBuf && depthW == w && depthH == h && line.depthTest) {
                    DrawLine2DDepthTested(buffer, w, h, x0, y0, x1, y1,
                                          z0, z1, depthBuf,
                                          ColorToC888(line.color));
                } else {
                    DrawLine2D(buffer, w, h, x0, y0, x1, y1,
                               ColorToC888(line.color));
                }
            }
        }
    }

    // --- Render boxes as 12 edges ---
    for (const auto& box : dd.GetBoxes()) {
        RenderBox(buffer, w, h, box,
                  camPos, invRot, camScale,
                  persp, fovScale, nearPlane, minC, maxC);
    }

    // --- Render spheres as 3 great-circle rings ---
    for (const auto& sphere : dd.GetSpheres()) {
        DrawSphere3D(buffer, w, h, sphere.center, sphere.radius,
                     ColorToC888(sphere.color),
                     camPos, invRot, camScale,
                     persp, fovScale, nearPlane, minC, maxC);
    }

    // --- Render screen-space text ---
    for (const auto& txt : dd.GetTexts()) {
        if (txt.screenSpace) {
            int px = static_cast<int>(txt.position.X * (w - 1));
            int py = static_cast<int>(txt.position.Y * (h - 1));
            int sc = static_cast<int>(txt.scale > 0 ? txt.scale : 1);
            BlitText(buffer, w, h, px, py, txt.text.CStr(),
                     ColorToC888(txt.color), sc);
        } else {
            int sx, sy;
            if (ProjectPoint(txt.position, camPos, invRot, camScale,
                             persp, fovScale, nearPlane,
                             minC, maxC, w, h, sx, sy)) {
                int sc = static_cast<int>(txt.scale > 0 ? txt.scale : 1);
                BlitText(buffer, w, h, sx, sy, txt.text.CStr(),
                         ColorToC888(txt.color), sc);
            }
        }
    }
}

// --- RenderWireframe ---------------------------------------

void DebugRenderer::RenderWireframe(Scene* scene, CameraBase* camera,
                                    Color888 color) {
    if (!scene || !camera) return;

    IPixelGroup* pg = camera->GetPixelGroup();
    if (!pg) return;
    Color888* buffer = pg->GetColorBuffer();
    if (!buffer) return;

    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    int w = static_cast<int>(maxC.X - minC.X + 1);
    int h = static_cast<int>(maxC.Y - minC.Y + 1);
    if (w <= 0 || h <= 0) return;

    bool persp = camera->IsPerspective();
    float fovScale = 0.0f;
    float nearPlane = camera->GetNearPlane();
    if (persp) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        fovScale = 1.0f / Mathematics::Tan(fovRad * 0.5f);
    }

    camera->GetTransform()->SetBaseRotation(
        camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation()
                            .Multiply(camera->GetLookOffset());
    Quaternion invRot = lookDir.Conjugate();
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D camScale = camera->GetTransform()->GetScale();

    for (unsigned int i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (!mesh || !mesh->IsEnabled()) continue;
        ITriangleGroup* tg = mesh->GetTriangleGroup();
        if (!tg) continue;

        for (uint32_t j = 0; j < tg->GetTriangleCount(); ++j) {
            const Triangle3D& tri = tg->GetTriangles()[j];
            int ax, ay, bx, by, cx, cy;
            bool a = ProjectPoint(*tri.p1, camPos, invRot, camScale,
                                  persp, fovScale, nearPlane,
                                  minC, maxC, w, h, ax, ay);
            bool b = ProjectPoint(*tri.p2, camPos, invRot, camScale,
                                  persp, fovScale, nearPlane,
                                  minC, maxC, w, h, bx, by);
            bool c = ProjectPoint(*tri.p3, camPos, invRot, camScale,
                                  persp, fovScale, nearPlane,
                                  minC, maxC, w, h, cx, cy);
            if (a && b) DrawLine2D(buffer, w, h, ax, ay, bx, by, color);
            if (b && c) DrawLine2D(buffer, w, h, bx, by, cx, cy, color);
            if (c && a) DrawLine2D(buffer, w, h, cx, cy, ax, ay, color);
        }
    }
}

// --- RenderOverlay -----------------------------------------

void DebugRenderer::RenderOverlay(Color888* buffer, int w, int h,
                                  const DebugOverlayStats& stats,
                                  Color888 color) {
    if (!buffer || w <= 0 || h <= 0) return;

    char line[64];
    int y = 1;

    snprintf(line, sizeof(line), "FPS:%d", static_cast<int>(stats.fps + 0.5f));
    BlitText(buffer, w, h, 1, y, line, color, 1);
    y += 9;

    snprintf(line, sizeof(line), "%.1fms", stats.frameTimeMs);
    BlitText(buffer, w, h, 1, y, line, color, 1);
    y += 9;

    if (stats.meshCount > 0) {
        snprintf(line, sizeof(line), "M:%d T:%d", stats.meshCount, stats.triCount);
        BlitText(buffer, w, h, 1, y, line, color, 1);
        y += 9;
    }

    if (stats.particleCount > 0) {
        snprintf(line, sizeof(line), "P:%d", stats.particleCount);
        BlitText(buffer, w, h, 1, y, line, color, 1);
    }
}

// --- Sphere Wireframe --------------------------------------

void DebugRenderer::DrawSphere3D(Color888* buffer, int w, int h,
                                 const Vector3D& center, float radius,
                                 Color888 color,
                                 const Vector3D& camPos,
                                 const Quaternion& invRot,
                                 const Vector3D& camScale,
                                 bool persp, float fovScale, float nearPlane,
                                 const Vector2D& minC, const Vector2D& maxC,
                                 int segments) {
    float step = 6.28318530f / segments; // 2*pi / segments
    for (int ring = 0; ring < 3; ++ring) {
        int prevX = 0, prevY = 0;
        bool prevOk = false;
        for (int i = 0; i <= segments; ++i) {
            float a = i * step;
            float ca = Mathematics::Cos(a) * radius;
            float sa = Mathematics::Sin(a) * radius;
            Vector3D pt;
            if (ring == 0) pt = center + Vector3D(ca, sa, 0);       // XY
            else if (ring == 1) pt = center + Vector3D(ca, 0, sa);  // XZ
            else pt = center + Vector3D(0, ca, sa);                  // YZ
            int sx, sy;
            bool ok = ProjectPoint(pt, camPos, invRot, camScale,
                                   persp, fovScale, nearPlane,
                                   minC, maxC, w, h, sx, sy);
            if (ok && prevOk) {
                DrawLine2D(buffer, w, h, prevX, prevY, sx, sy, color);
            }
            prevX = sx; prevY = sy; prevOk = ok;
        }
    }
}

// --- Physics Debug Visualization ---------------------------

void DebugRenderer::RenderPhysicsColliders(PhysicsWorld* world, CameraBase* camera,
                                           Color888 sphereColor,
                                           Color888 boxColor,
                                           Color888 capsuleColor) {
    if (!world || !camera) return;

    IPixelGroup* pg = camera->GetPixelGroup();
    if (!pg) return;
    Color888* buffer = pg->GetColorBuffer();
    if (!buffer) return;

    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    int w = static_cast<int>(maxC.X - minC.X + 1);
    int h = static_cast<int>(maxC.Y - minC.Y + 1);
    if (w <= 0 || h <= 0) return;

    bool persp = camera->IsPerspective();
    float fovScale = 0.0f, nearPlane = camera->GetNearPlane();
    if (persp) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        fovScale = 1.0f / Mathematics::Tan(fovRad * 0.5f);
    }
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());
    Quaternion invRot = lookDir.Conjugate();
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D camScale = camera->GetTransform()->GetScale();

    for (int i = 0; i < world->GetBodyCount(); ++i) {
        RigidBody* body = world->GetBody(i);
        if (!body) continue;
        Collider* col = body->GetCollider();
        if (!col || !col->IsEnabled()) continue;

        Vector3D pos = col->GetPosition();
        switch (col->GetType()) {
            case ColliderType::Sphere: {
                auto* sc = static_cast<SphereCollider*>(col);
                DrawSphere3D(buffer, w, h, pos, sc->GetRadius(), sphereColor,
                             camPos, invRot, camScale, persp, fovScale, nearPlane, minC, maxC);
                break;
            }
            case ColliderType::Box: {
                auto* bc = static_cast<BoxCollider*>(col);
                Vector3D sz = bc->GetSize();
                Vector3D half(sz.X * 0.5f, sz.Y * 0.5f, sz.Z * 0.5f);
                DebugBox db;
                db.center = pos;
                db.extents = half;
                db.color = Color(boxColor.R / 255.0f, boxColor.G / 255.0f, boxColor.B / 255.0f);
                RenderBox(buffer, w, h, db, camPos, invRot, camScale,
                          persp, fovScale, nearPlane, minC, maxC);
                break;
            }
            case ColliderType::Capsule: {
                auto* cc = static_cast<CapsuleCollider*>(col);
                float r = cc->GetRadius();
                float halfH = cc->GetHeight() * 0.5f - r;
                Vector3D top = pos + Vector3D(0, halfH, 0);
                Vector3D bot = pos - Vector3D(0, halfH, 0);
                // Draw top and bottom hemisphere circles + connecting lines
                DrawSphere3D(buffer, w, h, top, r, capsuleColor,
                             camPos, invRot, camScale, persp, fovScale, nearPlane, minC, maxC, 12);
                DrawSphere3D(buffer, w, h, bot, r, capsuleColor,
                             camPos, invRot, camScale, persp, fovScale, nearPlane, minC, maxC, 12);
                // 4 connecting lines
                for (int j = 0; j < 4; ++j) {
                    float a = j * 1.5707963f; // pi/2
                    Vector3D off(Mathematics::Cos(a) * r, 0, Mathematics::Sin(a) * r);
                    int x0, y0, x1, y1;
                    if (ProjectPoint(top + off, camPos, invRot, camScale,
                                     persp, fovScale, nearPlane, minC, maxC, w, h, x0, y0) &&
                        ProjectPoint(bot + off, camPos, invRot, camScale,
                                     persp, fovScale, nearPlane, minC, maxC, w, h, x1, y1)) {
                        DrawLine2D(buffer, w, h, x0, y0, x1, y1, capsuleColor);
                    }
                }
                break;
            }
            default: break;
        }
    }
}

void DebugRenderer::RenderPhysicsContacts(PhysicsWorld* world, CameraBase* camera,
                                          float normalLength,
                                          Color888 pointColor,
                                          Color888 normalColor) {
    if (!world || !camera) return;

    IPixelGroup* pg = camera->GetPixelGroup();
    if (!pg) return;
    Color888* buffer = pg->GetColorBuffer();
    if (!buffer) return;

    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    int w = static_cast<int>(maxC.X - minC.X + 1);
    int h = static_cast<int>(maxC.Y - minC.Y + 1);
    if (w <= 0 || h <= 0) return;

    bool persp = camera->IsPerspective();
    float fovScale = 0.0f, nearPlane = camera->GetNearPlane();
    if (persp) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        fovScale = 1.0f / Mathematics::Tan(fovRad * 0.5f);
    }
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());
    Quaternion invRot = lookDir.Conjugate();
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D camScale = camera->GetTransform()->GetScale();

    for (const auto& contact : world->GetDebugContacts()) {
        int cx, cy;
        if (ProjectPoint(contact.contactPoint, camPos, invRot, camScale,
                         persp, fovScale, nearPlane, minC, maxC, w, h, cx, cy)) {
            // Draw contact point as small crosshair
            DrawLine2D(buffer, w, h, cx - 2, cy, cx + 2, cy, pointColor);
            DrawLine2D(buffer, w, h, cx, cy - 2, cx, cy + 2, pointColor);

            // Draw normal arrow
            Vector3D tip = contact.contactPoint + contact.normal * normalLength;
            int tx, ty;
            if (ProjectPoint(tip, camPos, invRot, camScale,
                             persp, fovScale, nearPlane, minC, maxC, w, h, tx, ty)) {
                DrawLine2D(buffer, w, h, cx, cy, tx, ty, normalColor);
            }
        }
    }
}

// --- Depth & Normal Visualization --------------------------

void DebugRenderer::RenderDepthView(Color888* buffer, int w, int h) {
    const float* depth = Rasterizer::GetDepthBuffer();
    if (!depth || w <= 0 || h <= 0) return;

    int dw = Rasterizer::GetDebugWidth();
    int dh = Rasterizer::GetDebugHeight();
    int total = (w < dw ? w : dw) * (h < dh ? h : dh);

    // Find min/max depth for normalization
    float minZ = Mathematics::FLTMAX;
    float maxZ = -Mathematics::FLTMAX;
    for (int i = 0; i < total; ++i) {
        if (depth[i] < 1e10f) { // skip infinity (no geometry)
            if (depth[i] < minZ) minZ = depth[i];
            if (depth[i] > maxZ) maxZ = depth[i];
        }
    }
    if (maxZ <= minZ) maxZ = minZ + 1.0f;
    float range = maxZ - minZ;

    for (int i = 0; i < total; ++i) {
        uint8_t v = 0;
        if (depth[i] < 1e10f) {
            float norm = 1.0f - (depth[i] - minZ) / range; // near=1, far=0
            v = static_cast<uint8_t>(norm * 255.0f);
        }
        buffer[i] = Color888(v, v, v);
    }
}

void DebugRenderer::RenderNormalView(Color888* buffer, int w, int h) {
    const float* normals = Rasterizer::GetNormalBuffer();
    if (!normals || w <= 0 || h <= 0) return;

    int dw = Rasterizer::GetDebugWidth();
    int dh = Rasterizer::GetDebugHeight();
    int total = (w < dw ? w : dw) * (h < dh ? h : dh);

    for (int i = 0; i < total; ++i) {
        float nx = normals[i * 3 + 0];
        float ny = normals[i * 3 + 1];
        float nz = normals[i * 3 + 2];
        // Map [-1,1] -> [0,255]
        buffer[i] = Color888(
            static_cast<uint8_t>((nx * 0.5f + 0.5f) * 255.0f),
            static_cast<uint8_t>((ny * 0.5f + 0.5f) * 255.0f),
            static_cast<uint8_t>((nz * 0.5f + 0.5f) * 255.0f)
        );
    }
}

// --- Low-level drawing primitives --------------------------

void DebugRenderer::DrawLine2D(Color888* buffer, int w, int h,
                               int x0, int y0, int x1, int y1,
                               Color888 color) {
    // Cohen-Sutherland clipping would be ideal, but simple
    // per-pixel bounds check is MCU-friendly and sufficient.
    int dx = x1 - x0 < 0 ? -(x1 - x0) : (x1 - x0);
    int dy = y1 - y0 < 0 ? -(y1 - y0) : (y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) {
            buffer[y0 * w + x0] = color;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void DebugRenderer::DrawLine2DDepthTested(Color888* buffer, int w, int h,
                                          int x0, int y0, int x1, int y1,
                                          float z0, float z1,
                                          const float* depthBuf,
                                          Color888 color) {
    int dx = x1 - x0 < 0 ? -(x1 - x0) : (x1 - x0);
    int dy = y1 - y0 < 0 ? -(y1 - y0) : (y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int steps = dx > dy ? dx : dy;
    if (steps == 0) steps = 1;
    // Use linear Z interpolation to match the rasterizer, which also blends
    // vertex depths linearly in screen space. Perspective-correct (1/z) pulls
    // depths toward the near end and causes lines to falsely pass depth tests.
    float zStep = (z1 - z0) / static_cast<float>(steps);
    float z = z0;

    for (;;) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) {
            int idx = y0 * w + x0;
            if (z < depthBuf[idx] + 0.01f * z) {
                buffer[idx] = color;
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
        z += zStep;
    }
}

void DebugRenderer::BlitText(Color888* buffer, int w, int h,
                             int startX, int startY,
                             const char* text, Color888 color, int scale) {
    if (!text || scale < 1) return;
    int cx = startX;
    for (; *text; ++text) {
        if (*text == '\n') {
            startY += 8 * scale + scale;
            cx = startX;
            continue;
        }
        const uint8_t* glyph = Characters::GetCharacter(*text);
        if (!glyph) { cx += 8 * scale; continue; }
        for (int row = 0; row < 8; ++row) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (1 << (7 - col))) {
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            int px = cx + col * scale + sx;
                            int py = startY + row * scale + sy;
                            if (px >= 0 && px < w && py >= 0 && py < h) {
                                buffer[py * w + px] = color;
                            }
                        }
                    }
                }
            }
        }
        cx += 8 * scale;
    }
}

// --- Projection --------------------------------------------

bool DebugRenderer::ProjectLineClipped(const Vector3D& worldA, const Vector3D& worldB,
                                       const Vector3D& camPos,
                                       const Quaternion& invCamRot,
                                       const Vector3D& camScale,
                                       bool perspective, float fovScale,
                                       float nearPlane,
                                       const Vector2D& minC, const Vector2D& maxC,
                                       int w, int h,
                                       int& outX0, int& outY0,
                                       int& outX1, int& outY1,
                                       float& outZ0, float& outZ1) {
    outZ0 = outZ1 = 1.0f;
    if (!perspective) {
        return ProjectPoint(worldA, camPos, invCamRot, camScale,
                            perspective, fovScale, nearPlane,
                            minC, maxC, w, h, outX0, outY0) &&
               ProjectPoint(worldB, camPos, invCamRot, camScale,
                            perspective, fovScale, nearPlane,
                            minC, maxC, w, h, outX1, outY1);
    }

    // Transform both endpoints to camera space
    Vector3D relA = worldA - camPos;
    Vector3D projA = invCamRot.RotateVector(relA);
    if (camScale.X != 0) projA.X /= camScale.X;
    if (camScale.Y != 0) projA.Y /= camScale.Y;
    if (camScale.Z != 0) projA.Z /= camScale.Z;
    float zA = -projA.Z;

    Vector3D relB = worldB - camPos;
    Vector3D projB = invCamRot.RotateVector(relB);
    if (camScale.X != 0) projB.X /= camScale.X;
    if (camScale.Y != 0) projB.Y /= camScale.Y;
    if (camScale.Z != 0) projB.Z /= camScale.Z;
    float zB = -projB.Z;

    // Both behind camera
    if (zA < nearPlane && zB < nearPlane) return false;

    // Clip against near plane
    if (zA < nearPlane) {
        float t = (nearPlane - zA) / (zB - zA);
        projA.X += (projB.X - projA.X) * t;
        projA.Y += (projB.Y - projA.Y) * t;
        zA = nearPlane;
    } else if (zB < nearPlane) {
        float t = (nearPlane - zB) / (zA - zB);
        projB.X += (projA.X - projB.X) * t;
        projB.Y += (projA.Y - projB.Y) * t;
        zB = nearPlane;
    }

    float centerX = (minC.X + maxC.X) * 0.5f;
    float centerY = (minC.Y + maxC.Y) * 0.5f;
    float halfH   = (maxC.Y - minC.Y) * 0.5f;
    if (halfH == 0) return false;

    float dzA = zA > nearPlane ? zA : nearPlane;
    float dzB = zB > nearPlane ? zB : nearPlane;

    float sx0 = centerX + projA.X * fovScale / dzA * halfH;
    float sy0 = centerY + projA.Y * fovScale / dzA * halfH;
    float sx1 = centerX + projB.X * fovScale / dzB * halfH;
    float sy1 = centerY + projB.Y * fovScale / dzB * halfH;

    // Liang-Barsky parametric viewport clip so lines with off-screen endpoints
    // that still pass through the viewport are correctly drawn. Without this,
    // a grid line rotated ~45° often has both endpoints projected far outside
    // the extended ±1-viewport check and is silently dropped.
    float t0 = 0.0f, t1 = 1.0f;
    float dsx = sx1 - sx0, dsy = sy1 - sy0;
    auto clip = [](float p, float q, float& t0, float& t1) -> bool {
        if (p == 0.0f) return q >= 0.0f;
        float t = q / p;
        if (p < 0.0f) { if (t > t1) return false; if (t > t0) t0 = t; }
        else           { if (t < t0) return false; if (t < t1) t1 = t; }
        return true;
    };
    if (!clip(-dsx, sx0,           t0, t1)) return false;
    if (!clip( dsx, (w - 1) - sx0, t0, t1)) return false;
    if (!clip(-dsy, sy0,           t0, t1)) return false;
    if (!clip( dsy, (h - 1) - sy0, t0, t1)) return false;

    outX0 = static_cast<int>(sx0 + t0 * dsx + 0.5f);
    outY0 = static_cast<int>(sy0 + t0 * dsy + 0.5f);
    outX1 = static_cast<int>(sx0 + t1 * dsx + 0.5f);
    outY1 = static_cast<int>(sy0 + t1 * dsy + 0.5f);
    outZ0 = dzA + t0 * (dzB - dzA);
    outZ1 = dzA + t1 * (dzB - dzA);

    return true;
}

bool DebugRenderer::ProjectPoint(const Vector3D& worldPos,
                                 const Vector3D& camPos,
                                 const Quaternion& invCamRot,
                                 const Vector3D& camScale,
                                 bool perspective, float fovScale,
                                 float nearPlane,
                                 const Vector2D& minC, const Vector2D& maxC,
                                 int w, int h,
                                 int& outX, int& outY) {
    // World -> camera space
    Vector3D rel = worldPos - camPos;
    Vector3D proj = invCamRot.RotateVector(rel);
    if (camScale.X != 0) proj.X /= camScale.X;
    if (camScale.Y != 0) proj.Y /= camScale.Y;
    if (camScale.Z != 0) proj.Z /= camScale.Z;

    // Camera looks down -Z; negate for positive forward depth (matches rasterizer)
    float negZ = -proj.Z;

    // Behind camera?
    if (perspective && negZ < nearPlane) return false;

    float centerX = (minC.X + maxC.X) * 0.5f;
    float centerY = (minC.Y + maxC.Y) * 0.5f;
    float halfW   = (maxC.X - minC.X) * 0.5f;
    float halfH   = (maxC.Y - minC.Y) * 0.5f;
    if (halfW == 0 || halfH == 0) return false;

    float screenX, screenY;
    if (perspective) {
        float dz = negZ > nearPlane ? negZ : nearPlane;
        screenX = centerX + proj.X * fovScale / dz * halfH;
        screenY = centerY + proj.Y * fovScale / dz * halfH;
    } else {
        screenX = proj.X;
        screenY = proj.Y;
    }

    outX = static_cast<int>(screenX + 0.5f);
    outY = static_cast<int>(screenY + 0.5f);

    // Allow some off-screen margin for line clipping
    return (outX >= -w && outX < 2 * w && outY >= -h && outY < 2 * h);
}

// --- Private utilities -------------------------------------

Color888 DebugRenderer::ColorToC888(const Color& c) {
    return Color888(
        static_cast<uint8_t>(Mathematics::Min(c.r * 255.0f, 255.0f)),
        static_cast<uint8_t>(Mathematics::Min(c.g * 255.0f, 255.0f)),
        static_cast<uint8_t>(Mathematics::Min(c.b * 255.0f, 255.0f))
    );
}

void DebugRenderer::RenderBox(Color888* buffer, int w, int h,
                              const DebugBox& box,
                              const Vector3D& camPos,
                              const Quaternion& invRot,
                              const Vector3D& camScale,
                              bool persp, float fovScale, float nearPlane,
                              const Vector2D& minC, const Vector2D& maxC) {
    Color888 c = ColorToC888(box.color);
    Vector3D e = box.extents;
    Vector3D corners[8] = {
        { -e.X, -e.Y, -e.Z }, {  e.X, -e.Y, -e.Z },
        {  e.X,  e.Y, -e.Z }, { -e.X,  e.Y, -e.Z },
        { -e.X, -e.Y,  e.Z }, {  e.X, -e.Y,  e.Z },
        {  e.X,  e.Y,  e.Z }, { -e.X,  e.Y,  e.Z }
    };
    // Transform and offset to world
    for (int i = 0; i < 8; ++i) {
        corners[i] = corners[i] + box.center;
    }
    // 12 edges: bottom(0-3), top(4-7), verticals
    static const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    int sx[8], sy[8];
    bool vis[8];
    for (int i = 0; i < 8; ++i) {
        vis[i] = ProjectPoint(corners[i], camPos, invRot, camScale,
                              persp, fovScale, nearPlane,
                              minC, maxC, w, h, sx[i], sy[i]);
    }
    for (const auto& edge : edges) {
        if (vis[edge[0]] && vis[edge[1]]) {
            DrawLine2D(buffer, w, h,
                       sx[edge[0]], sy[edge[0]],
                       sx[edge[1]], sy[edge[1]], c);
        }
    }
}

// === Reflection ===

KL_DEFINE_FIELDS(DebugRenderer)
KL_END_FIELDS

KL_DEFINE_METHODS(DebugRenderer)
KL_END_METHODS

KL_DEFINE_DESCRIBE(DebugRenderer)
KL_END_DESCRIBE(DebugRenderer)

} // namespace koilo
