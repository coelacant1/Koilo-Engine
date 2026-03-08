// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/debug/debugdraw.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <algorithm>
#include <cmath>

namespace koilo {

// Static color definitions
const Color koilo::Color::White(1.0f, 1.0f, 1.0f, 1.0f);
const Color koilo::Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
const Color koilo::Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color koilo::Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color koilo::Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color koilo::Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
const Color koilo::Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);
const Color koilo::Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Color koilo::Color::Orange(1.0f, 0.5f, 0.0f, 1.0f);
const Color koilo::Color::Purple(0.5f, 0.0f, 0.5f, 1.0f);
const Color koilo::Color::Gray(0.5f, 0.5f, 0.5f, 1.0f);

// Static member initialization
DebugDraw* koilo::DebugDraw::instance = nullptr;

// === DebugDraw Implementation ===

koilo::DebugDraw::DebugDraw() : enabled(true) {
}

DebugDraw& koilo::DebugDraw::GetInstance() {
    if (instance == nullptr) {
        instance = new DebugDraw();
    }
    return *instance;
}

void koilo::DebugDraw::Update() {
    if (!enabled) return;
    float deltaTime = TimeManager::GetInstance().GetDeltaTime();

    // Expire timed primitives; duration <= 0 means one-frame (remove immediately)
    auto expireLine = [&](DebugLine& l) -> bool {
        if (l.duration <= 0.0f) return true;
        l.duration -= deltaTime;
        return l.duration <= 0.0f;
    };
    lines.erase(std::remove_if(lines.begin(), lines.end(), expireLine), lines.end());

    auto expireSphere = [&](DebugSphere& s) -> bool {
        if (s.duration <= 0.0f) return true;
        s.duration -= deltaTime;
        return s.duration <= 0.0f;
    };
    spheres.erase(std::remove_if(spheres.begin(), spheres.end(), expireSphere), spheres.end());

    auto expireBox = [&](DebugBox& b) -> bool {
        if (b.duration <= 0.0f) return true;
        b.duration -= deltaTime;
        return b.duration <= 0.0f;
    };
    boxes.erase(std::remove_if(boxes.begin(), boxes.end(), expireBox), boxes.end());

    auto expireText = [&](DebugText& t) -> bool {
        if (t.duration <= 0.0f) return true;
        t.duration -= deltaTime;
        return t.duration <= 0.0f;
    };
    texts.erase(std::remove_if(texts.begin(), texts.end(), expireText), texts.end());
}

void koilo::DebugDraw::Clear() {
    lines.clear();
    spheres.clear();
    boxes.clear();
    texts.clear();
}

// === Line Drawing ===

void koilo::DebugDraw::DrawLine(const Vector3D& start, const Vector3D& end, const Color& color,
                         float duration, bool depthTest) {
    if (!enabled) return;

    DebugLine line;
    line.start = start;
    line.end = end;
    line.color = color;
    line.duration = duration;
    line.depthTest = depthTest;

    lines.push_back(line);
}

void koilo::DebugDraw::DrawRay(const Vector3D& origin, const Vector3D& direction, float length,
                        const Color& color, float duration, bool depthTest) {
    if (!enabled) return;

    Vector3D normalizedDir = direction.Normal();
    Vector3D end = origin.Add(normalizedDir.Multiply(length));

    DrawLine(origin, end, color, duration, depthTest);
}

void koilo::DebugDraw::DrawArrow(const Vector3D& start, const Vector3D& end, const Color& color,
                          float duration, bool depthTest) {
    if (!enabled) return;

    // Stub: simplified arrow - just draw the main line
    DrawLine(start, end, color, duration, depthTest);
}

// === Shape Drawing ===

void koilo::DebugDraw::DrawSphere(const Vector3D& center, float radius, const Color& color,
                           float duration, bool depthTest, DebugDrawMode mode) {
    if (!enabled) return;

    DebugSphere sphere;
    sphere.center = center;
    sphere.radius = radius;
    sphere.color = color;
    sphere.duration = duration;
    sphere.depthTest = depthTest;
    sphere.mode = mode;

    spheres.push_back(sphere);
}

void koilo::DebugDraw::DrawBox(const Vector3D& center, const Vector3D& extents, const Color& color,
                        float duration, bool depthTest, DebugDrawMode mode) {
    if (!enabled) return;

    DebugBox box;
    box.center = center;
    box.extents = extents;
    box.transform = Matrix4x4();  // Identity for axis-aligned
    box.color = color;
    box.duration = duration;
    box.depthTest = depthTest;
    box.mode = mode;

    boxes.push_back(box);
}

void koilo::DebugDraw::DrawOrientedBox(const Vector3D& center, const Vector3D& extents, const Matrix4x4& transform,
                                const Color& color, float duration, bool depthTest, DebugDrawMode mode) {
    if (!enabled) return;

    DebugBox box;
    box.center = center;
    box.extents = extents;
    box.transform = transform;
    box.color = color;
    box.duration = duration;
    box.depthTest = depthTest;
    box.mode = mode;

    boxes.push_back(box);
}

void koilo::DebugDraw::DrawCube(const Vector3D& center, float size, const Color& color,
                         float duration, bool depthTest) {
    if (!enabled) return;

    Vector3D extents(size * 0.5f, size * 0.5f, size * 0.5f);
    DrawBox(center, extents, color, duration, depthTest);
}

// === Coordinate Systems ===

void koilo::DebugDraw::DrawAxes(const Vector3D& position, float scale, float duration, bool depthTest) {
    if (!enabled) return;

    DrawLine(position, position.Add(Vector3D(scale, 0, 0)), koilo::Color::Red, duration, depthTest);
    DrawLine(position, position.Add(Vector3D(0, scale, 0)), koilo::Color::Green, duration, depthTest);
    DrawLine(position, position.Add(Vector3D(0, 0, scale)), koilo::Color::Blue, duration, depthTest);
}

void koilo::DebugDraw::DrawGrid(const Vector3D& center, float size, int divisions,
                         const Color& color, float duration, bool depthTest) {
    if (!enabled) return;

    float halfSize = size * 0.5f;
    float step = size / divisions;
    float tolerance = step * 0.01f;

    // Brighter color for major gridlines (every 10th division)
    Color majorColor(
        color.r * 1.6f > 1.0f ? 1.0f : color.r * 1.6f,
        color.g * 1.6f > 1.0f ? 1.0f : color.g * 1.6f,
        color.b * 1.6f > 1.0f ? 1.0f : color.b * 1.6f,
        color.a);

    // Draw lines parallel to X axis (varying Z)
    for (int i = 0; i <= divisions; ++i) {
        float z = -halfSize + (i * step);
        float worldZ = center.Z + z;
        Vector3D start(center.X - halfSize, center.Y, center.Z + z);
        Vector3D end(center.X + halfSize, center.Y, center.Z + z);

        if (std::fabs(worldZ) < tolerance) {
            DrawLine(start, end, koilo::Color::Red, duration, depthTest);
        } else if (i % 10 == 0) {
            DrawLine(start, end, majorColor, duration, depthTest);
        } else {
            DrawLine(start, end, color, duration, depthTest);
        }
    }

    // Draw lines parallel to Z axis (varying X)
    for (int i = 0; i <= divisions; ++i) {
        float x = -halfSize + (i * step);
        float worldX = center.X + x;
        Vector3D start(center.X + x, center.Y, center.Z - halfSize);
        Vector3D end(center.X + x, center.Y, center.Z + halfSize);

        if (std::fabs(worldX) < tolerance) {
            DrawLine(start, end, koilo::Color::Green, duration, depthTest);
        } else if (i % 10 == 0) {
            DrawLine(start, end, majorColor, duration, depthTest);
        } else {
            DrawLine(start, end, color, duration, depthTest);
        }
    }
}

// === Text Drawing ===

void koilo::DebugDraw::DrawText(const UString& text, const Vector3D& position, const Color& color,
                         float duration, float scale) {
    if (!enabled) return;

    DebugText debugText;
    debugText.text = text;
    debugText.position = position;
    debugText.color = color;
    debugText.duration = duration;
    debugText.screenSpace = false;
    debugText.scale = scale;

    texts.push_back(debugText);
}

void koilo::DebugDraw::DrawScreenText(const UString& text, float screenX, float screenY,
                               const Color& color, float duration, float scale) {
    if (!enabled) return;

    DebugText debugText;
    debugText.text = text;
    debugText.position = Vector3D(screenX, screenY, 0);
    debugText.color = color;
    debugText.duration = duration;
    debugText.screenSpace = true;
    debugText.scale = scale;

    texts.push_back(debugText);
}

} // namespace koilo
