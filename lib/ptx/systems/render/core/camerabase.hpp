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
#include "ipixelgroup.hpp" // Include for pixel group interface.
#include "../../../core/math/transform.hpp" // Include for transformation utilities.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class CameraBase
 * @brief Base class for managing camera properties and transformations.
 *
 * The CameraBase class provides an abstract interface for camera operations,
 * including retrieving camera bounds and transformations, and managing associated
 * pixel groups.
 */
class CameraBase {
protected:
    Transform* transform; ///< Pointer to the camera's transformation data.
    CameraLayout* cameraLayout; ///< Pointer to the camera's layout information.
    Quaternion lookOffset; ///< Look offset for the camera's orientation.
    bool is2D = false; ///< Flag indicating whether the camera operates in 2D mode.

public:
    /**
     * @brief Default constructor.
     */
    CameraBase();

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

    PTX_BEGIN_FIELDS(CameraBase)
        /* TODO: PTX_FIELD(CameraBase, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(CameraBase)
        PTX_METHOD_AUTO(CameraBase, GetCameraMinCoordinate, "Get camera min coordinate"),
        PTX_METHOD_AUTO(CameraBase, GetCameraMaxCoordinate, "Get camera max coordinate"),
        PTX_METHOD_AUTO(CameraBase, GetCameraCenterCoordinate, "Get camera center coordinate"),
        PTX_METHOD_AUTO(CameraBase, GetCameraTransformMin, "Get camera transform min"),
        PTX_METHOD_AUTO(CameraBase, GetCameraTransformMax, "Get camera transform max"),
        PTX_METHOD_AUTO(CameraBase, GetCameraTransformCenter, "Get camera transform center"),
        PTX_METHOD_AUTO(CameraBase, GetPixelGroup, "Get pixel group"),
        PTX_METHOD_AUTO(CameraBase, GetCameraLayout, "Get camera layout"),
        PTX_METHOD_AUTO(CameraBase, GetTransform, "Get transform"),
        PTX_METHOD_AUTO(CameraBase, Is2D, "Is2 d"),
        PTX_METHOD_AUTO(CameraBase, Set2D, "Set2 d"),
        PTX_METHOD_AUTO(CameraBase, SetLookOffset, "Set look offset"),
        PTX_METHOD_AUTO(CameraBase, GetLookOffset, "Get look offset")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(CameraBase)
        PTX_CTOR0(CameraBase)
    PTX_END_DESCRIBE(CameraBase)
    
};
