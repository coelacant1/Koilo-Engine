/**
 * @file Project.h
 * @brief Declares the Project class for managing animations, rendering, and display operations.
 *
 * This file defines the Project class, which integrates camera management, scene rendering,
 * and display updates while maintaining performance statistics.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "../systems/render/engine/renderer.hpp" // Include for rendering engine.
#include "../systems/scene/scene.hpp" // Include for scene management.
#include "../systems/render/core/cameramanager.hpp" // Include for camera management.
#include "../systems/hardware/controller.hpp" // Include for controller operations.
#include "../core/signal/filter/runningaveragefilter.hpp" // Include for filtering utilities.
#include "../core/platform/time.hpp"
#include "../core/platform/console.hpp"
#include "../registry/reflect_macros.hpp"

/**
 * @class Project
 * @brief Manages animations, rendering, and display operations.
 *
 * The Project class integrates various subsystems including camera management, scene rendering,
 * and display updates. It also tracks and reports performance metrics such as frame rate and
 * individual operation times.
 */
class Project {
protected:
    CameraManager* cameras; ///< Pointer to the CameraManager for managing cameras.
    Controller* controller; ///< Pointer to the Controller for controlling the display.
    Scene scene; ///< The Scene object representing the rendered environment.

    RunningAverageFilter<50> avgFPS = RunningAverageFilter<50>(0.05f); ///< Running average filter for frame rate calculation.

    long previousAnimationTime = 0; ///< Time of the previous animation frame in microseconds.
    long previousRenderTime = 0; ///< Time of the previous render frame in microseconds.
    long previousDisplayTime = 0; ///< Time of the previous display frame in microseconds.
    float fade = 0.0f; ///< Fade parameter for animations.
    float animationTime = 0.0f; ///< Time spent on animation in milliseconds.
    float renderTime = 0.0f; ///< Time spent on rendering in milliseconds.
    float displayTime = 0.0f; ///< Time spent on display in milliseconds.

    /**
     * @brief Updates the project state based on the given ratio.
     *
     * This method is intended to be overridden by derived classes to define
     * specific update logic for animations or state transitions.
     *
     * @param ratio A float representing the interpolation ratio for updates.
     */
    virtual void Update(float ratio) = 0;

    /**
     * @brief Starts the render timer for measuring render performance.
     */
    void RenderStartTimer();

    /**
     * @brief Stops the render timer and records the elapsed time.
     */
    void RenderEndTimer();

public:

    Project() : cameras(nullptr), controller(nullptr), scene(0) {}

    /**
     * @brief Constructs a Project with specified camera manager and controller.
     *
     * @param cameras Pointer to the CameraManager managing the cameras.
     * @param controller Pointer to the Controller managing the display.
     * @param numObjects Number of objects to initialize in the scene.
     */
    Project(CameraManager* cameras, Controller* controller, uint8_t numObjects);

    void SetCameraManager(CameraManager* cm) { cameras = cm; }

    void SetController(Controller* c)        { controller = c; }

    /**
     * @brief Retrieves the time spent on animations.
     *
     * @return The animation time in milliseconds.
     */
    float GetAnimationTime();

    /**
     * @brief Retrieves the time spent on rendering.
     *
     * @return The render time in milliseconds.
     */
    float GetRenderTime();

    /**
     * @brief Retrieves the time spent on display operations.
     *
     * @return The display time in milliseconds.
     */
    float GetDisplayTime();

    /**
     * @brief Retrieves the current frame rate.
     *
     * @return The frame rate in frames per second (FPS).
     */
    float GetFrameRate();

    /**
     * @brief Initializes the project.
     *
     * This method is intended to be overridden by derived classes to define
     * specific initialization logic for the project.
     */
    virtual void Initialize() = 0;

    /**
     * @brief Animates the project state based on the given ratio.
     *
     * @param ratio A float representing the interpolation ratio for animations.
     */
    void Animate(float ratio);

    /**
     * @brief Renders the scene.
     */
    void Render();

    /**
     * @brief Updates the display with the rendered content.
     */
    void Display();

    /**
     * @brief Prints performance statistics such as frame rate and operation times.
     */
    void PrintStats();

    PTX_BEGIN_FIELDS(Project)
        /* TODO: PTX_FIELD(Project, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Project)
        PTX_METHOD_AUTO(Project, SetCameraManager, "Set camera manager"),
        PTX_METHOD_AUTO(Project, SetController, "Set controller"),
        PTX_METHOD_AUTO(Project, GetAnimationTime, "Get animation time"),
        PTX_METHOD_AUTO(Project, GetRenderTime, "Get render time"),
        PTX_METHOD_AUTO(Project, GetDisplayTime, "Get display time"),
        PTX_METHOD_AUTO(Project, GetFrameRate, "Get frame rate"),
        PTX_METHOD_AUTO(Project, Initialize, "Initialize"),
        PTX_METHOD_AUTO(Project, Animate, "Animate"),
        PTX_METHOD_AUTO(Project, Render, "Render"),
        PTX_METHOD_AUTO(Project, Display, "Display"),
        PTX_METHOD_AUTO(Project, PrintStats, "Print stats")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Project)
        PTX_CTOR0(Project),
        PTX_CTOR(Project, CameraManager *, Controller *, uint8_t)
    PTX_END_DESCRIBE(Project)
    
};
