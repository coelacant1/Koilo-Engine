/**
 * @file Camera.h
 * @brief Declares the Camera template class for managing camera behavior and pixel groups.
 *
 * This file defines the Camera class, which extends CameraBase to provide functionality
 * for camera operations and managing associated PixelGroup instances.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "camerabase.hpp" // Include for base camera functionality.
#include "../../../registry/reflect_macros.hpp"

class IPixelGroup;

/**
 * @class Camera
 * @brief Manages camera behavior and pixel groups.
 *
 * The Camera class extends CameraBase and incorporates pixel group handling, allowing
 * for advanced camera operations, including retrieving pixel data and coordinate transformations.
 *
 */
class Camera : public CameraBase {
private:
    IPixelGroup* pixelGroup = nullptr; ///< Pointer to the associated PixelGroup instance.
    Vector2D maxC; ///< Cached maximum coordinate of the camera.
    Vector2D minC; ///< Cached minimum coordinate of the camera.
    bool calculatedMax = false; ///< Indicates if the maximum coordinate has been calculated.
    bool calculatedMin = false; ///< Indicates if the minimum coordinate has been calculated.

public:
    /**
     * @brief Constructs a Camera with a transform and pixel group.
     *
     * @param transform Pointer to the Transform associated with the camera.
    * @param pixelGroup Pointer to the PixelGroup associated with the camera.
     */
    Camera(Transform* transform, IPixelGroup* pixelGroup);

    /**
     * @brief Constructs a Camera with a transform, camera layout, and pixel group.
     *
     * @param transform Pointer to the Transform associated with the camera.
     * @param cameraLayout Pointer to the CameraLayout for the camera.
    * @param pixelGroup Pointer to the PixelGroup associated with the camera.
     */
    Camera(Transform* transform, CameraLayout* cameraLayout, IPixelGroup* pixelGroup);

    /**
     * @brief Virtual default destructor.
     */
    ~Camera() override = default;

    /**
     * @brief Retrieves the associated PixelGroup.
     *
    * @return Pointer to the PixelGroup.
     */
    IPixelGroup* GetPixelGroup() override;

    /**
     * @brief Retrieves the minimum coordinate of the camera.
     *
     * @return The minimum coordinate as a Vector2D.
     */
    Vector2D GetCameraMinCoordinate() override;

    /**
     * @brief Retrieves the maximum coordinate of the camera.
     *
     * @return The maximum coordinate as a Vector2D.
     */
    Vector2D GetCameraMaxCoordinate() override;

    /**
     * @brief Retrieves the center coordinate of the camera.
     *
     * @return The center coordinate as a Vector2D.
     */
    Vector2D GetCameraCenterCoordinate() override;

    /**
     * @brief Retrieves the minimum transform of the camera.
     *
     * @return The minimum transform as a Vector3D.
     */
    Vector3D GetCameraTransformMin() override;

    /**
     * @brief Retrieves the maximum transform of the camera.
     *
     * @return The maximum transform as a Vector3D.
     */
    Vector3D GetCameraTransformMax() override;

    /**
     * @brief Retrieves the center transform of the camera.
     *
     * @return The center transform as a Vector3D.
     */
    Vector3D GetCameraTransformCenter() override;

    PTX_BEGIN_FIELDS(Camera)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Camera)
        PTX_METHOD_AUTO(Camera, GetPixelGroup, "Get pixel group"),
        PTX_METHOD_AUTO(Camera, GetCameraMinCoordinate, "Get camera min coordinate"),
        PTX_METHOD_AUTO(Camera, GetCameraMaxCoordinate, "Get camera max coordinate"),
        PTX_METHOD_AUTO(Camera, GetCameraCenterCoordinate, "Get camera center coordinate"),
        PTX_METHOD_AUTO(Camera, GetCameraTransformMin, "Get camera transform min"),
        PTX_METHOD_AUTO(Camera, GetCameraTransformMax, "Get camera transform max"),
        PTX_METHOD_AUTO(Camera, GetCameraTransformCenter, "Get camera transform center")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Camera)
        PTX_CTOR(Camera, Transform *, IPixelGroup *),
        PTX_CTOR(Camera, Transform *, CameraLayout *, IPixelGroup *)
    PTX_END_DESCRIBE(Camera)

};