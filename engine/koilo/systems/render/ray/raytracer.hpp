// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file RayTracer.h
 * @brief Ray tracing rendering system for 3D scenes.
 *
 * The RayTracer class provides ray tracing functionality for rendering 3D scenes,
 * similar to the Rasterizer but using ray casting for image synthesis. Supports
 * shadows, reflections, and other ray tracing effects.
 *
 * @date 22/12/2024
 * @version 2.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/scene.hpp>
#include "rayintersection.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct RayTraceSettings
 * @brief Configurable settings for ray traced rendering.
 */
struct RayTraceSettings {
    int maxBounces = 3;            ///< Maximum ray bounce depth for reflections
    int samplesPerPixel = 1;       ///< Samples per pixel for anti-aliasing
    bool shadows = true;           ///< Enable shadow ray casting
    bool reflections = false;      ///< Enable reflection rays
    float ambientLight = 0.1f;     ///< Ambient light intensity (0-1)
    Color888 backgroundColor = Color888(50, 50, 50);  ///< Background color for misses

    RayTraceSettings() = default;

    KL_BEGIN_FIELDS(RayTraceSettings)
        KL_FIELD(RayTraceSettings, maxBounces, "Max bounces", -2147483648, 2147483647),
        KL_FIELD(RayTraceSettings, samplesPerPixel, "Samples per pixel", -2147483648, 2147483647),
        KL_FIELD(RayTraceSettings, shadows, "Shadows", 0, 1),
        KL_FIELD(RayTraceSettings, reflections, "Reflections", 0, 1),
        KL_FIELD(RayTraceSettings, ambientLight, "Ambient light", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(RayTraceSettings, backgroundColor, "Background color", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RayTraceSettings)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RayTraceSettings)
        KL_CTOR0(RayTraceSettings)
    KL_END_DESCRIBE(RayTraceSettings)

};

/**
 * @class RayTracer
 * @brief Ray tracing rendering system.
 *
 * Provides static methods for ray tracing 3D scenes into 2D camera views.
 * Uses the unified Ray system for physics and rendering integration.
 */
class RayTracer {
private:
    static RayTraceSettings settings;  ///< Global ray tracing settings

public:
    /**
     * @brief Ray traces a 3D scene onto a 2D camera view.
     * 
     * Main rendering interface, similar to Rasterizer::Rasterize().
     * Renders the entire scene from the camera's perspective using ray tracing.
     *
     * @param scene Pointer to the 3D scene to render.
     * @param camera Pointer to the camera used for rendering.
     */
    static void RayTrace(Scene* scene, CameraBase* camera);

    /**
     * @brief Traces a single ray through the scene.
     * 
     * Recursively traces a ray, handling reflections and other effects.
     *
     * @param ray The ray to trace.
     * @param scene The scene to trace against.
     * @param depth Current recursion depth (for reflections).
     * @return Color calculated for this ray.
     */
    static Color888 TraceRay(const koilo::Ray& ray, Scene* scene, int depth = 0);

    /**
     * @brief Generates a camera ray for a specific pixel.
     * 
     * Creates a ray from the camera origin through the specified pixel
     * on the image plane.
     *
     * @param camera Camera to generate ray from.
     * @param x Pixel X coordinate.
     * @param y Pixel Y coordinate.
     * @return Ray from camera through pixel.
     */
    static koilo::Ray GenerateCameraRay(CameraBase* camera, int x, int y);

    /**
     * @brief Gets the current ray trace settings.
     */
    static RayTraceSettings& GetSettings() { return settings; }

    /**
     * @brief Sets ray trace settings.
     */
    static void SetSettings(const RayTraceSettings& newSettings) { settings = newSettings; }

    KL_BEGIN_FIELDS(RayTracer)
        /* No reflected fields - static class. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(RayTracer)
        KL_SMETHOD_AUTO(RayTracer::RayTrace, "Ray trace"),
        KL_SMETHOD_AUTO(RayTracer::TraceRay, "Trace ray"),
        KL_SMETHOD_AUTO(RayTracer::GenerateCameraRay, "Generate camera ray")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RayTracer)
        /* No reflected ctors - static class. */
    KL_END_DESCRIBE(RayTracer)
};

} // namespace koilo
