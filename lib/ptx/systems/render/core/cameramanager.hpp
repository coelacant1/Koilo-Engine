/**
 * @file CameraManager.h
 * @brief Declares the CameraManager class for managing multiple camera instances.
 *
 * This file defines the CameraManager class, which provides functionality to manage
 * and retrieve multiple CameraBase objects.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include "camerabase.hpp" // Include for base camera functionality.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class CameraManager
 * @brief Manages multiple CameraBase objects.
 *
 * The CameraManager class provides a centralized interface for handling
 * multiple camera instances, including retrieval of camera data and count.
 */
class CameraManager {
protected:
    CameraBase** cameras; ///< Array of pointers to CameraBase objects.
    uint8_t count; ///< Number of CameraBase objects managed.

public:
    /**
     * @brief Constructs a CameraManager object.
     *
     * @param cameras Pointer to an array of CameraBase objects.
     * @param count The number of cameras in the array.
     */
    CameraManager(CameraBase** cameras, uint8_t count);

    /**
     * @brief Retrieves the array of CameraBase objects.
     *
     * @return A pointer to the array of CameraBase objects.
     */
    CameraBase** GetCameras();

    /**
     * @brief Retrieves the count of cameras managed by the CameraManager.
     *
     * @return The number of CameraBase objects.
     */
    uint8_t GetCameraCount();

    PTX_BEGIN_FIELDS(CameraManager)
        /* TODO: PTX_FIELD(CameraManager, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(CameraManager)
        PTX_METHOD_AUTO(CameraManager, GetCameras, "Get cameras"),
        PTX_METHOD_AUTO(CameraManager, GetCameraCount, "Get camera count")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(CameraManager)
        PTX_CTOR(CameraManager, CameraBase **, uint8_t)
    PTX_END_DESCRIBE(CameraManager)
    
};
