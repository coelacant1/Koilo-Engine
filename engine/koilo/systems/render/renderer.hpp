/**
 * @file renderer.hpp
 * @brief Declares the RenderingEngine class for rendering and display operations.
 *
 * This file defines the RenderingEngine class, which provides static methods
 * for rasterizing scenes and managing display operations, such as rendering a white fill.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "../core/cameramanager.hpp" // Include for camera management.
#include "../../scene/scene.hpp" // Include for scene management.
#include "../raster/rasterizer.hpp" // Include for rasterization operations.
#include "../ray/raytracer.hpp" // Include for display test utilities.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class RenderingEngine
 * @brief Provides static methods for rendering and display operations.
 *
 * The RenderingEngine class offers functionality for rasterizing scenes using cameras
 * and managing display operations such as filling the screen with a white color.
 */
class RenderingEngine {
public:
    /**
     * @brief Rasterizes the given scene using the cameras managed by the CameraManager.
     *
     * This method iterates through all cameras in the CameraManager and rasterizes the scene
     * for each camera. If the scene includes a post-processing effect, it applies the effect
     * to the corresponding pixel group.
     *
     * @param scene Pointer to the Scene to be rasterized.
     * @param cameraManager Pointer to the CameraManager managing the cameras.
     */
    static void Rasterize(Scene* scene, CameraManager* cameraManager);

    /**
     * @brief RayTraces the given scene using the cameras managed by the CameraManager.
     * 
     * 
     * 
     * @param scene Pointer to the Scene to be rasterized.
     * @param cameraManager Pointer to the CameraManager managing the cameras.
     */
    static void RayTrace(Scene* scene, CameraManager* cameraManager);

    PTX_BEGIN_FIELDS(RenderingEngine)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(RenderingEngine)
        PTX_SMETHOD_AUTO(RenderingEngine::Rasterize, "Rasterize"),
        PTX_SMETHOD_AUTO(RenderingEngine::RayTrace, "Ray trace")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(RenderingEngine)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(RenderingEngine)

};
