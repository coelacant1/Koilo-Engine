// SPDX-License-Identifier: GPL-3.0-or-later
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

#include <koilo/systems/scene/camera/cameramanager.hpp> // Include for camera management.
#include <koilo/systems/scene/scene.hpp> // Include for scene management.
#include <koilo/systems/render/raster/rasterizer.hpp> // Include for rasterization operations.
#include <koilo/systems/render/ray/raytracer.hpp> // Include for display test utilities.
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

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

    KL_BEGIN_FIELDS(RenderingEngine)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(RenderingEngine)
        KL_SMETHOD_AUTO(RenderingEngine::Rasterize, "Rasterize"),
        KL_SMETHOD_AUTO(RenderingEngine::RayTrace, "Ray trace")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RenderingEngine)
        /* No reflected ctors. */
    KL_END_DESCRIBE(RenderingEngine)

};

} // namespace koilo
