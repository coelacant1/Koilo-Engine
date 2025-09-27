/**
 * @file rasterizer.h
 * @brief Provides functionality for rasterizing 3D scenes into 2D camera views.
 *
 * The Rasterizer class handles rendering a 3D scene by projecting it onto a 2D camera view.
 * It supports triangle-based rasterization with optional acceleration structures for efficiency.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't, moepforfreedom
 */

#pragma once

#include "../../scene/scene.hpp"
#include "../../../core/geometry/spatial/quadtree.hpp"
#include "../core/camerabase.hpp"
#include "../../../core/color/rgbcolor.hpp"
#include "helpers/rastertriangle2d.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class Rasterizer
 * @brief Provides static methods for rasterizing 3D scenes into 2D camera views.
 */
class Rasterizer {
private:
    /**
     * @brief Finds the correct color for a single pixel by testing against a list of triangles.
     *
     * @param candidate_triangles A C-style array of pointers to candidate 2D raster triangles.
     * @param count The number of triangles in the candidates array.
     * @param pixel_coord The 2D coordinate of the pixel being rendered.
     * @return The calculated RGBColor for the pixel. Returns black if no intersection.
     */
    static RGBColor RasterizePixel(RasterTriangle2D** candidate_triangles, unsigned short count, const Vector2D& pixel_coord);

public:
    /**
     * @brief Renders an entire scene from the perspective of a given camera.
     * @param scene The scene containing meshes and materials.
     * @param camera The camera defining the viewpoint and projection.
     */
    static void Rasterize(Scene* scene, CameraBase* camera);

    PTX_BEGIN_FIELDS(Rasterizer)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Rasterizer)
        PTX_SMETHOD_AUTO(Rasterizer::Rasterize, "Rasterize")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Rasterizer)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(Rasterizer)

};