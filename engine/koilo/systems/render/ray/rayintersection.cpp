// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/ray/rayintersection.hpp>
#include <koilo/systems/scene/meshraycast.hpp>
#include <limits>
#include <algorithm>

namespace koilo {

RayHitInfo koilo::RayIntersection::IntersectScene(const Ray& ray, Scene* scene) {
    RayHitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = std::numeric_limits<float>::max();

    if (!scene) {
        return closestHit;
    }

    Mesh** meshes = scene->GetMeshes();
    if (!meshes) {
        return closestHit;
    }

    const unsigned int objectCount = scene->GetMeshCount();

    for (unsigned int i = 0; i < objectCount; ++i) {
        Mesh* mesh = meshes[i];
        if (!mesh || !mesh->IsEnabled()) {
            continue;
        }

        RayHitInfo hit = IntersectMesh(ray, mesh);

        if (hit.hit && hit.distance < closestHit.distance) {
            closestHit = hit;
        }
    }

    return closestHit;
}

RayHitInfo koilo::RayIntersection::IntersectMesh(const Ray& ray, Mesh* mesh) {
    RayHitInfo hitInfo;
    hitInfo.hit = false;

    if (!mesh || !mesh->IsEnabled()) {
        return hitInfo;
    }

    // Use MeshRaycast for triangle intersection
    RaycastHit physicsHit;
    if (MeshRaycast::Raycast(ray, mesh, physicsHit, std::numeric_limits<float>::max(), true)) {
        hitInfo.hit = true;
        hitInfo.distance = physicsHit.distance;
        hitInfo.point = physicsHit.point;
        hitInfo.normal = physicsHit.normal;
        hitInfo.mesh = mesh;
        hitInfo.material = mesh->GetMaterial();

        // Get base color from material if available
        // For now, use a default color
        // TODO: Query material for color/texture at hit point
        hitInfo.color = Color888(200, 200, 200);
    }

    return hitInfo;
}

Color888 koilo::RayIntersection::CalculateLighting(const RayHitInfo& hit,
                                           Scene* scene,
                                           const Ray& incomingRay,
                                           float ambientLight) {
    if (!hit.hit) {
        return Color888(0, 0, 0);
    }

    // Start with ambient lighting
    Color888 finalColor = hit.color.Scale(static_cast<uint8_t>(ambientLight * 255));

    // TODO: Iterate through scene lights
    // For now, use a simple directional light from above
    Vector3D lightDir(0, -1, 0);  // Light from above
    lightDir = lightDir.Normal();

    // Lambertian diffuse shading
    float diffuse = std::max(0.0f, hit.normal.DotProduct(lightDir * -1.0f));

    // Apply diffuse lighting
    uint8_t brightness = static_cast<uint8_t>(std::min(255.0f, ambientLight * 255 + diffuse * 255));
    finalColor = hit.color.Scale(brightness);

    // TODO: Specular highlights
    // TODO: Shadow rays
    // TODO: Reflection/refraction

    return finalColor;
}

bool koilo::RayIntersection::IsLightVisible(const Vector3D& point,
                                    const Vector3D& lightPosition,
                                    Scene* scene) {
    // Create ray from point to light
    Ray shadowRay = Ray::FromPoints(point, lightPosition);

    // Small offset to prevent self-intersection
    Vector3D offsetOrigin = point + shadowRay.direction * 0.001f;
    shadowRay.origin = offsetOrigin;

    float distanceToLight = (lightPosition - point).Magnitude();

    // Check if anything blocks the path to light
    RayHitInfo hit = IntersectScene(shadowRay, scene);

    if (hit.hit && hit.distance < distanceToLight) {
        return false;  // Something is blocking the light
    }

    return true;  // Light is visible
}

} // namespace koilo
