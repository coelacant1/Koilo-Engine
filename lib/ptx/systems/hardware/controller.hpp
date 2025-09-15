/**
 * @file Controller.h
 * @brief Declares the Controller base class for managing lighting controllers.
 *
 * This file defines the Controller class, which serves as a base class for
 * managing brightness and display operations for various lighting controllers.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "../../core/math/mathematics.hpp" // Include for mathematical utilities.
#include "../render/core/cameramanager.hpp" // Include for camera management.
#include "../../registry/reflect_macros.hpp"

/**
 * @class Controller
 * @brief Base class for managing brightness and display operations of lighting controllers.
 *
 * The Controller class provides an abstract interface and shared functionality
 * for managing lighting systems, including brightness adjustment and display updates.
 */
class Controller {
protected:
    CameraManager* cameras; ///< Pointer to the CameraManager for managing camera data.

    /**
     * @brief Constructs a Controller with specified parameters.
     *
     * @param cameras Pointer to the CameraManager.
     */
    Controller(CameraManager* cameras);

public:
    /**
     * @brief Initializes the controller.
     */
    virtual void Initialize() = 0;

    /**
     * @brief Updates and displays content on the lighting system.
     */
    virtual void Display() = 0;


    PTX_BEGIN_FIELDS(Controller)
        /* TODO: PTX_FIELD(Controller, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Controller)
        PTX_METHOD_AUTO(Controller, Initialize, "Initialize"),
        PTX_METHOD_AUTO(Controller, Display, "Display")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Controller)
        /* TODO: PTX_CTOR0(Controller) or PTX_CTOR(Controller, ...) */
    PTX_END_DESCRIBE(Controller)
    
};
