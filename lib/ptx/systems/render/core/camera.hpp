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
#include "pixelgroup.hpp" // Include for pixel group management.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class Camera
 * @brief Manages camera behavior and pixel groups.
 *
 * The Camera class extends CameraBase and incorporates pixel group handling, allowing
 * for advanced camera operations, including retrieving pixel data and coordinate transformations.
 *
 * @tparam pixelCount The total number of pixels managed by the camera.
 */
template<size_t pixelCount>
class Camera : public CameraBase {
private:
    PixelGroup<pixelCount>* pixelGroup; ///< Pointer to the associated PixelGroup instance.
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
    Camera(Transform* transform, PixelGroup<pixelCount>* pixelGroup);

    /**
     * @brief Constructs a Camera with a transform, camera layout, and pixel group.
     *
     * @param transform Pointer to the Transform associated with the camera.
     * @param cameraLayout Pointer to the CameraLayout for the camera.
     * @param pixelGroup Pointer to the PixelGroup associated with the camera.
     */
    Camera(Transform* transform, CameraLayout* cameraLayout, PixelGroup<pixelCount>* pixelGroup);

    /**
     * @brief Retrieves the associated PixelGroup.
     *
     * @return Pointer to the PixelGroup.
     */
    PixelGroup<pixelCount>* GetPixelGroup() override;

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

    /* NOTE: Camera is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(Camera)
        /* TODO: PTX_FIELD(Camera, member, "Doc", min, max) */
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
        PTX_CTOR(Camera, Transform *, PixelGroup<pixelCount> *),
        PTX_CTOR(Camera, Transform *, CameraLayout *, PixelGroup<pixelCount> *)
    PTX_END_DESCRIBE(Camera)
    
};

#include "camera.tpp" // Include the template implementation.