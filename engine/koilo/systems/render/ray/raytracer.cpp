// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/ray/raytracer.hpp>
#include <koilo/systems/render/ray/rayintersection.hpp>
#include <algorithm>


namespace koilo {

// Static member initialization
RayTraceSettings koilo::RayTracer::settings;

void koilo::RayTracer::RayTrace(Scene* scene, CameraBase* camera) {
    if (!scene || !camera) {
        return;
    }

    IPixelGroup* pixelGroup = camera->GetPixelGroup();
    if (!pixelGroup) {
        return;
    }

    Vector2D size = pixelGroup->GetSize();
    const int width = static_cast<int>(size.X);
    const int height = static_cast<int>(size.Y);
    Color888* pixels = pixelGroup->GetColors();
    if (!pixels) {
        return;
    }

    // Render each pixel
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color888 pixelColor(0, 0, 0);

            // Multi-sampling for anti-aliasing
            for (int sample = 0; sample < settings.samplesPerPixel; ++sample) {
                // Generate ray for this pixel (add jitter for anti-aliasing if samples > 1)
                koilo::Ray ray = GenerateCameraRay(camera, x, y);

                // Trace ray through scene
                Color888 sampleColor = TraceRay(ray, scene, 0);

                // Accumulate color
                pixelColor.R = std::min(255, pixelColor.R + sampleColor.R / settings.samplesPerPixel);
                pixelColor.G = std::min(255, pixelColor.G + sampleColor.G / settings.samplesPerPixel);
                pixelColor.B = std::min(255, pixelColor.B + sampleColor.B / settings.samplesPerPixel);
            }

            // Set pixel color in camera buffer
            pixels[y * width + x] = pixelColor;
        }
    }
}

Color888 koilo::RayTracer::TraceRay(const koilo::Ray& ray, Scene* scene, int depth) {
    // Check recursion depth
    if (depth >= settings.maxBounces) {
        return settings.backgroundColor;
    }

    // Find intersection with scene
    koilo::RayHitInfo hit = koilo::RayIntersection::IntersectScene(ray, scene);

    if (!hit.hit) {
        // No hit - return background color
        return settings.backgroundColor;
    }

    // Calculate lighting at hit point
    Color888 color = koilo::RayIntersection::CalculateLighting(hit, scene, ray, settings.ambientLight);

    // Add reflection if enabled
    if (settings.reflections && depth < settings.maxBounces - 1) {
        // Calculate reflection ray
        Vector3D reflectionDir = ray.direction - hit.normal * (2.0f * ray.direction.DotProduct(hit.normal));
        reflectionDir = reflectionDir.Normal();

        // Offset origin slightly to avoid self-intersection
        Vector3D reflectionOrigin = hit.point + hit.normal * 0.001f;
        koilo::Ray reflectionRay(reflectionOrigin, reflectionDir);

        // Trace reflection ray
        Color888 reflectionColor = TraceRay(reflectionRay, scene, depth + 1);

        // Blend reflection with base color (simple 50/50 blend for now)
        // TODO: Use material properties for reflection amount
        color.R = std::min(255, (color.R + reflectionColor.R) / 2);
        color.G = std::min(255, (color.G + reflectionColor.G) / 2);
        color.B = std::min(255, (color.B + reflectionColor.B) / 2);
    }

    return color;
}

koilo::Ray koilo::RayTracer::GenerateCameraRay(CameraBase* camera, int x, int y) {
    if (!camera) {
        return koilo::Ray();
    }

    IPixelGroup* pixelGroup = camera->GetPixelGroup();
    if (!pixelGroup) {
        return koilo::Ray();
    }

    Vector2D size = pixelGroup->GetSize();
    const int width = static_cast<int>(size.X);
    const int height = static_cast<int>(size.Y);

    // Convert pixel coordinates to normalized device coordinates [-1, 1]
    float ndcX = (2.0f * x / width) - 1.0f;
    float ndcY = 1.0f - (2.0f * y / height);  // Flip Y

    // Calculate aspect ratio
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    // Calculate ray direction based on field of view
    // Assume 90 degree FOV for simplicity (can be made configurable)
    float fov = 90.0f;
    float tanHalfFov = std::tan(fov * 0.5f * 3.14159f / 180.0f);

    // Ray direction in camera space
    Vector3D rayDir(
        ndcX * aspectRatio * tanHalfFov,
        ndcY * tanHalfFov,
        1.0f  // Forward in camera space
    );
    rayDir = rayDir.Normal();

    // Transform ray direction by camera rotation
    Transform* cameraTransform = camera->GetTransform();
    Quaternion cameraRotation = cameraTransform->GetRotation();
    Vector3D worldRayDir = cameraRotation.RotateVector(rayDir);

    // Create ray from camera position
    Vector3D cameraPos = cameraTransform->GetPosition();

    return koilo::Ray(cameraPos, worldRayDir);
}

} // namespace koilo
