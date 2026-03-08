#include <ptx/debug/debugdraw.hpp>
#include <algorithm>
#include <cmath>

namespace ptx {

// Static color definitions
const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Orange(1.0f, 0.5f, 0.0f, 1.0f);
const Color Color::Purple(0.5f, 0.0f, 0.5f, 1.0f);
const Color Color::Gray(0.5f, 0.5f, 0.5f, 1.0f);

// Static member initialization
DebugDraw* DebugDraw::instance = nullptr;

// === DebugDraw Implementation ===

DebugDraw::DebugDraw() : enabled(true) {
}

DebugDraw& DebugDraw::GetInstance() {
    if (instance == nullptr) {
        instance = new DebugDraw();
    }
    return *instance;
}

void DebugDraw::Update(float deltaTime) {
    if (!enabled) return;

    // Remove expired debug draws
    lines.erase(
        std::remove_if(lines.begin(), lines.end(),
            [deltaTime](DebugLine& line) {
                if (line.duration > 0.0f) {
                    line.duration -= deltaTime;
                    return line.duration <= 0.0f;
                }
                return line.duration == 0.0f;  // One-frame draws
            }),
        lines.end()
    );

    spheres.erase(
        std::remove_if(spheres.begin(), spheres.end(),
            [deltaTime](DebugSphere& sphere) {
                if (sphere.duration > 0.0f) {
                    sphere.duration -= deltaTime;
                    return sphere.duration <= 0.0f;
                }
                return sphere.duration == 0.0f;
            }),
        spheres.end()
    );

    boxes.erase(
        std::remove_if(boxes.begin(), boxes.end(),
            [deltaTime](DebugBox& box) {
                if (box.duration > 0.0f) {
                    box.duration -= deltaTime;
                    return box.duration <= 0.0f;
                }
                return box.duration == 0.0f;
            }),
        boxes.end()
    );

    texts.erase(
        std::remove_if(texts.begin(), texts.end(),
            [deltaTime](DebugText& text) {
                if (text.duration > 0.0f) {
                    text.duration -= deltaTime;
                    return text.duration <= 0.0f;
                }
                return text.duration == 0.0f;
            }),
        texts.end()
    );
}

void DebugDraw::Clear() {
    lines.clear();
    spheres.clear();
    boxes.clear();
    texts.clear();
}

// === Line Drawing ===

void DebugDraw::DrawLine(const Vector3D& start, const Vector3D& end, const Color& color,
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

void DebugDraw::DrawRay(const Vector3D& origin, const Vector3D& direction, float length,
                        const Color& color, float duration, bool depthTest) {
    if (!enabled) return;

    Vector3D normalizedDir = direction;
    normalizedDir.normalize();
    Vector3D end = origin + (normalizedDir * length);

    DrawLine(origin, end, color, duration, depthTest);
}

void DebugDraw::DrawArrow(const Vector3D& start, const Vector3D& end, const Color& color,
                          float duration, bool depthTest) {
    if (!enabled) return;

    // Draw main line
    DrawLine(start, end, color, duration, depthTest);

    // Draw arrowhead
    Vector3D direction = end - start;
    direction.normalize();

    // Create perpendicular vectors for arrowhead
    Vector3D up(0, 1, 0);
    if (std::abs(direction.dot(up)) > 0.99f) {
        up = Vector3D(1, 0, 0);
    }

    Vector3D right = direction.cross(up);
    right.normalize();
    up = right.cross(direction);
    up.normalize();

    float arrowSize = (end - start).length() * 0.1f;

    Vector3D arrowBase = end - (direction * arrowSize);
    Vector3D arrowLeft = arrowBase + (right * arrowSize * 0.3f);
    Vector3D arrowRight = arrowBase - (right * arrowSize * 0.3f);
    Vector3D arrowUp = arrowBase + (up * arrowSize * 0.3f);
    Vector3D arrowDown = arrowBase - (up * arrowSize * 0.3f);

    DrawLine(end, arrowLeft, color, duration, depthTest);
    DrawLine(end, arrowRight, color, duration, depthTest);
    DrawLine(end, arrowUp, color, duration, depthTest);
    DrawLine(end, arrowDown, color, duration, depthTest);
}

// === Shape Drawing ===

void DebugDraw::DrawSphere(const Vector3D& center, float radius, const Color& color,
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

    // Also draw wireframe using lines for immediate visualization
    const int segments = 16;
    const float angleStep = 2.0f * 3.14159265359f / segments;

    // Draw three circles (XY, XZ, YZ planes)
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        // XY plane circle
        Vector3D p1(center.x + radius * std::cos(angle1), center.y + radius * std::sin(angle1), center.z);
        Vector3D p2(center.x + radius * std::cos(angle2), center.y + radius * std::sin(angle2), center.z);
        DrawLine(p1, p2, color, duration, depthTest);

        // XZ plane circle
        p1 = Vector3D(center.x + radius * std::cos(angle1), center.y, center.z + radius * std::sin(angle1));
        p2 = Vector3D(center.x + radius * std::cos(angle2), center.y, center.z + radius * std::sin(angle2));
        DrawLine(p1, p2, color, duration, depthTest);

        // YZ plane circle
        p1 = Vector3D(center.x, center.y + radius * std::cos(angle1), center.z + radius * std::sin(angle1));
        p2 = Vector3D(center.x, center.y + radius * std::cos(angle2), center.z + radius * std::sin(angle2));
        DrawLine(p1, p2, color, duration, depthTest);
    }
}

void DebugDraw::DrawBox(const Vector3D& center, const Vector3D& extents, const Color& color,
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

    // Draw wireframe box using lines
    Vector3D corners[8] = {
        center + Vector3D(-extents.x, -extents.y, -extents.z),
        center + Vector3D( extents.x, -extents.y, -extents.z),
        center + Vector3D( extents.x,  extents.y, -extents.z),
        center + Vector3D(-extents.x,  extents.y, -extents.z),
        center + Vector3D(-extents.x, -extents.y,  extents.z),
        center + Vector3D( extents.x, -extents.y,  extents.z),
        center + Vector3D( extents.x,  extents.y,  extents.z),
        center + Vector3D(-extents.x,  extents.y,  extents.z)
    };

    // Bottom face
    DrawLine(corners[0], corners[1], color, duration, depthTest);
    DrawLine(corners[1], corners[2], color, duration, depthTest);
    DrawLine(corners[2], corners[3], color, duration, depthTest);
    DrawLine(corners[3], corners[0], color, duration, depthTest);

    // Top face
    DrawLine(corners[4], corners[5], color, duration, depthTest);
    DrawLine(corners[5], corners[6], color, duration, depthTest);
    DrawLine(corners[6], corners[7], color, duration, depthTest);
    DrawLine(corners[7], corners[4], color, duration, depthTest);

    // Vertical edges
    DrawLine(corners[0], corners[4], color, duration, depthTest);
    DrawLine(corners[1], corners[5], color, duration, depthTest);
    DrawLine(corners[2], corners[6], color, duration, depthTest);
    DrawLine(corners[3], corners[7], color, duration, depthTest);
}

void DebugDraw::DrawOrientedBox(const Vector3D& center, const Vector3D& extents, const Matrix4x4& transform,
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

    // TODO: Transform corners by matrix and draw wireframe
    // For now, just draw axis-aligned box
    DrawBox(center, extents, color, duration, depthTest, mode);
}

void DebugDraw::DrawCube(const Vector3D& center, float size, const Color& color,
                         float duration, bool depthTest) {
    if (!enabled) return;

    Vector3D extents(size * 0.5f, size * 0.5f, size * 0.5f);
    DrawBox(center, extents, color, duration, depthTest);
}

// === Coordinate Systems ===

void DebugDraw::DrawAxes(const Vector3D& position, float scale, float duration, bool depthTest) {
    if (!enabled) return;

    DrawLine(position, position + Vector3D(scale, 0, 0), Color::Red, duration, depthTest);
    DrawLine(position, position + Vector3D(0, scale, 0), Color::Green, duration, depthTest);
    DrawLine(position, position + Vector3D(0, 0, scale), Color::Blue, duration, depthTest);
}

void DebugDraw::DrawGrid(const Vector3D& center, float size, int divisions,
                         const Color& color, float duration, bool depthTest) {
    if (!enabled) return;

    float halfSize = size * 0.5f;
    float step = size / divisions;

    // Draw lines parallel to X axis
    for (int i = 0; i <= divisions; ++i) {
        float z = -halfSize + (i * step);
        Vector3D start(center.x - halfSize, center.y, center.z + z);
        Vector3D end(center.x + halfSize, center.y, center.z + z);
        DrawLine(start, end, color, duration, depthTest);
    }

    // Draw lines parallel to Z axis
    for (int i = 0; i <= divisions; ++i) {
        float x = -halfSize + (i * step);
        Vector3D start(center.x + x, center.y, center.z - halfSize);
        Vector3D end(center.x + x, center.y, center.z + halfSize);
        DrawLine(start, end, color, duration, depthTest);
    }
}

// === Text Drawing ===

void DebugDraw::DrawText(const std::string& text, const Vector3D& position, const Color& color,
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

void DebugDraw::DrawScreenText(const std::string& text, float screenX, float screenY,
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

} // namespace ptx
