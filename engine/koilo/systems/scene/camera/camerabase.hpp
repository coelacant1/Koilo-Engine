// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file CameraBase.h
 * @brief Declares the CameraBase class for defining camera functionality.
 *
 * This file defines the CameraBase class, which serves as a base class for managing
 * camera transformations, layouts, and pixel groups in 2D or 3D space.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "cameralayout.hpp" // Include for camera layout management.
#include <koilo/systems/render/core/ipixelgroup.hpp> // Include for pixel group interface.
#include <koilo/core/math/transform.hpp> // Include for transformation utilities.
#include <koilo/core/color/color888.hpp> // Include for sky gradient colors.


namespace koilo {

/**
 * @class CameraBase
 * @brief Base class for managing camera properties and transformations.
 *
 * The CameraBase class provides an abstract interface for camera operations,
 * including retrieving camera bounds and transformations, and managing associated
 * pixel groups.
 */
class CameraBase {
public:
    enum class ProjectionType { ORTHOGRAPHIC, PERSPECTIVE };

protected:
    Transform* transform = nullptr; ///< Pointer to the camera's transformation data.
    CameraLayout* cameraLayout = nullptr; ///< Pointer to the camera's layout information.
    Quaternion lookOffset; ///< Look offset for the camera's orientation.
    bool is2D = false; ///< Flag indicating whether the camera operates in 2D mode.
    
    // Projection settings
    ProjectionType projectionType_ = ProjectionType::ORTHOGRAPHIC;
    float fov_ = 60.0f;          ///< Field of view in degrees (perspective only).
    float nearPlane_ = 0.1f;     ///< Near clipping plane.
    float farPlane_ = 1000.0f;   ///< Far clipping plane.
    bool backfaceCulling_ = false; ///< Enable screen-space backface culling.

    // Sky gradient
    bool hasSkyGradient_ = false;
    Color888 skyTop_{0, 0, 0};
    Color888 skyBottom_{0, 0, 0};

public:
    /**
     * @brief Default constructor.
     */
    CameraBase();

    /**
     * @brief Virtual destructor to allow derived cameras to clean up safely.
     */
    virtual ~CameraBase() = default;

    /**
     * @brief Retrieves the minimum coordinate of the camera in 2D space.
     *
     * @return The minimum coordinate as a Vector2D.
     */
    virtual Vector2D GetCameraMinCoordinate() = 0;

    /**
     * @brief Retrieves the maximum coordinate of the camera in 2D space.
     *
     * @return The maximum coordinate as a Vector2D.
     */
    virtual Vector2D GetCameraMaxCoordinate() = 0;

    /**
     * @brief Retrieves the center coordinate of the camera in 2D space.
     *
     * @return The center coordinate as a Vector2D.
     */
    virtual Vector2D GetCameraCenterCoordinate() = 0;

    /**
     * @brief Retrieves the minimum transform of the camera in 3D space.
     *
     * @return The minimum transform as a Vector3D.
     */
    virtual Vector3D GetCameraTransformMin() = 0;

    /**
     * @brief Retrieves the maximum transform of the camera in 3D space.
     *
     * @return The maximum transform as a Vector3D.
     */
    virtual Vector3D GetCameraTransformMax() = 0;

    /**
     * @brief Retrieves the center transform of the camera in 3D space.
     *
     * @return The center transform as a Vector3D.
     */
    virtual Vector3D GetCameraTransformCenter() = 0;

    /**
     * @brief Retrieves the associated pixel group.
     *
     * @return Pointer to the IPixelGroup interface.
     */
    virtual IPixelGroup* GetPixelGroup() = 0;

    /**
     * @brief Retrieves the camera's layout.
     *
     * @return Pointer to the CameraLayout.
     */
    CameraLayout* GetCameraLayout();

    /**
     * @brief Retrieves the camera's transformation data.
     *
     * @return Pointer to the Transform object.
     */
    Transform* GetTransform();

    /**
     * @brief Checks if the camera operates in 2D mode.
     *
     * @return True if the camera is in 2D mode, otherwise false.
     */
    bool Is2D();

    /**
     * @brief Sets the camera's 2D mode.
     *
     * @param is2D True to enable 2D mode, otherwise false.
     */
    void Set2D(bool is2D);

    /**
     * @brief Sets the camera's look offset.
     *
     * @param lookOffset The new look offset as a Quaternion.
     */
    void SetLookOffset(Quaternion lookOffset);

    /**
     * @brief Retrieves the camera's look offset.
     *
     * @return The look offset as a Quaternion.
     */
    Quaternion GetLookOffset();

    // Projection settings
    ProjectionType GetProjectionType() const { return projectionType_; }
    void SetProjectionType(ProjectionType type) { projectionType_ = type; }
    void SetPerspective(float fov, float nearPlane, float farPlane);
    float GetFOV() const { return fov_; }
    float GetNearPlane() const { return nearPlane_; }
    float GetFarPlane() const { return farPlane_; }
    bool IsPerspective() const { return projectionType_ == ProjectionType::PERSPECTIVE; }
    
    // Backface culling
    bool GetBackfaceCulling() const { return backfaceCulling_; }
    void SetBackfaceCulling(bool enabled) { backfaceCulling_ = enabled; }

    // Sky gradient background
    bool HasSkyGradient() const { return hasSkyGradient_; }
    Color888 GetSkyTop() const { return skyTop_; }
    Color888 GetSkyBottom() const { return skyBottom_; }

    /**
     * @brief Sets a vertical gradient background (top to bottom).
     * @param top Color at the top of the viewport.
     * @param bottom Color at the bottom of the viewport.
     */
    void SetSkyGradient(Color888 top, Color888 bottom) {
        skyTop_ = top;
        skyBottom_ = bottom;
        hasSkyGradient_ = true;
    }

    /**
     * @brief Disables the sky gradient (reverts to black background).
     */
    void ClearSkyGradient() { hasSkyGradient_ = false; }

};

} // namespace koilo
