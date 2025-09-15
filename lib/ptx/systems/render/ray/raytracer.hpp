/**
 * @file RayTracer.h
 * @brief Provides functionality for ray tracing 3D scenes into 2D camera views.
 *
 * The RayTracer class handles rendering a 3D scene by ...
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't, moepforfreedom
 */

#pragma once

#include "../../../core/math/transform.hpp"
#include "../../../core/math/quaternion.hpp"
#include "../core/camera.hpp"
#include "../../scene/scene.hpp"
#include "../../../core/geometry/2d/triangle.hpp"
#include "../../../core/geometry/spatial/quadtree.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class RayTracer
 * @brief Provides static methods for ray tracing 3D scenes into 2D camera views.
 */
class RayTracer {
private:
    static Quaternion rayDirection; ///< Direction of the ray cast for ray tracation.

    /**
     * @brief Determines the color of a pixel by checking which triangle it intersects.
     * @param triangles Array of 2D triangles to check against.
     * @param numTriangles Number of triangles in the array.
     * @param pixelRay The 2D pixel ray to test.
     * @return The color of the intersected triangle, or a default color if no intersection.
     */
    static RGBColor RayTracePixel(Triangle2D** triangles, int numTriangles, Vector2D pixelRay);

public:
    /**
     * @brief Ray traces a 3D scene onto a 2D camera view.
     * @param scene Pointer to the 3D scene to render.
     * @param camera Pointer to the camera used for projection.
     */
    static void RayTrace(Scene* scene, CameraBase* camera);

    PTX_BEGIN_FIELDS(RayTracer)
        /* TODO: PTX_FIELD(RayTracer, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(RayTracer)
        PTX_SMETHOD_AUTO(RayTracer::RayTrace, "Ray trace")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(RayTracer)
        /* TODO: PTX_CTOR0(RayTracer) or PTX_CTOR(RayTracer, ...) */
    PTX_END_DESCRIBE(RayTracer)
    
};
