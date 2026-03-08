// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file displaymanager.hpp
 * @brief Manager for multiple display backends.
 *
 * The DisplayManager handles routing cameras to displays, managing multiple
 * output devices, and coordinating frame presentation.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <map>
#include <memory>
#include <vector>
#include "idisplaybackend.hpp"
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class DisplayManager
 * @brief Manages multiple display backends and camera routing.
 *
 * Usage:
 * @code
 * DisplayManager manager;
 * auto backend = std::make_unique<OpenGLBackend>(1920, 1080);
 * int displayId = manager.AddDisplay(std::move(backend));
 * manager.RouteCamera(camera, displayId);
 * manager.PresentAll();
 * @endcode
 */
class DisplayManager {
public:
    /**
     * @brief Constructor.
     */
    DisplayManager();
    
    /**
     * @brief Destructor - shuts down all displays.
     */
    ~DisplayManager();
    
    // === Display Management ===
    
    /**
     * @brief Add a display backend.
     * @param backend Unique pointer to backend (ownership transferred).
     * @return Display ID for future operations, or -1 on failure.
     */
    int AddDisplay(std::unique_ptr<IDisplayBackend> backend);
    
    /**
     * @brief Add a display backend (raw pointer - ownership transferred to DisplayManager).
     * @param backend Raw pointer to backend (DisplayManager takes ownership).
     * @return Display ID for future operations, or -1 on failure.
     */
    int AddDisplayRaw(IDisplayBackend* backend);
    
    /**
     * @brief Remove a display backend.
     * @param displayId Display ID to remove.
     * @return True if display was removed.
     */
    bool RemoveDisplay(int displayId);
    
    /**
     * @brief Get display backend by ID.
     * @param displayId Display ID.
     * @return Pointer to backend, or nullptr if not found.
     */
    IDisplayBackend* GetDisplay(int displayId);
    
    /**
     * @brief Get number of displays.
     * @return Number of registered displays.
     */
    int GetDisplayCount() const;
    
    /**
     * @brief Get all display IDs.
     * @return Vector of display IDs.
     */
    std::vector<int> GetDisplayIds() const;
    
    // === Camera Routing ===
    
    /**
     * @brief Route a camera to a display.
     * @param camera Camera to route.
     * @param displayId Display to route to.
     * @return True if routing succeeded.
     */
    bool RouteCamera(CameraBase* camera, int displayId);
    
    /**
     * @brief Unroute a camera from its display.
     * @param camera Camera to unroute.
     * @return True if camera was unrouted.
     */
    bool UnrouteCamera(CameraBase* camera);
    
    /**
     * @brief Get display ID for a camera.
     * @param camera Camera to query.
     * @return Display ID, or -1 if camera is not routed.
     */
    int GetCameraDisplay(CameraBase* camera) const;
    
    /**
     * @brief Get camera routed to a display.
     * @param displayId Display ID.
     * @return Pointer to camera, or nullptr if none routed.
     */
    CameraBase* GetDisplayCamera(int displayId) const;
    
    // === Presentation ===
    
    /**
     * @brief Present all routed cameras to their displays.
     * @return Number of successful presentations.
     *
     * This iterates through all displays, constructs framebuffers from
     * their routed cameras, and calls Present() on each backend.
     */
    int PresentAll();
    
    /**
     * @brief Present a specific display.
     * @param displayId Display to present.
     * @return True if presentation succeeded.
     */
    bool Present(int displayId);
    
    /**
     * @brief Clear all displays.
     */
    void ClearAll();
    
    /**
     * @brief Clear a specific display.
     * @param displayId Display to clear.
     * @return True if clear succeeded.
     */
    bool Clear(int displayId);
    
    // === Configuration ===
    
    /**
     * @brief Enable/disable VSync for all displays.
     * @param enabled True to enable VSync.
     */
    void SetVSyncEnabled(bool enabled);
    
    /**
     * @brief Enable/disable automatic presentation after rendering.
     * @param enabled True to auto-present.
     *
     * If enabled, displays will be presented automatically when their
     * cameras are rendered. If disabled, you must call Present() manually.
     */
    void SetAutoPresent(bool enabled);
    
    /**
     * @brief Get VSync enabled state.
     */
    bool IsVSyncEnabled() const { return vsyncEnabled_; }
    
    /**
     * @brief Get auto-present enabled state.
     */
    bool IsAutoPresentEnabled() const { return autoPresent_; }

private:
    struct DisplayEntry {
        std::unique_ptr<IDisplayBackend> backend;
        CameraBase* camera;
        Framebuffer framebuffer;
        std::vector<uint8_t> pixelBuffer;
        std::vector<float> geometryBuffer;

        KL_BEGIN_FIELDS(DisplayEntry)
            KL_FIELD(DisplayEntry, backend, "Backend", -2147483648, 2147483647),
            KL_FIELD(DisplayEntry, camera, "Camera", 0, 0),
            KL_FIELD(DisplayEntry, framebuffer, "Framebuffer", 0, 0),
            KL_FIELD(DisplayEntry, pixelBuffer, "Pixel buffer", -2147483648, 2147483647),
            KL_FIELD(DisplayEntry, geometryBuffer, "Geometry buffer", -2147483648, 2147483647)
        KL_END_FIELDS

        KL_BEGIN_METHODS(DisplayEntry)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(DisplayEntry)
            /* No reflected ctors. */
        KL_END_DESCRIBE(DisplayEntry)

    };
    
    std::map<int, DisplayEntry> displays_;
    std::map<CameraBase*, int> cameraToDisplay_;
    int nextDisplayId_;
    bool vsyncEnabled_;
    bool autoPresent_;
    
    // Helper to build framebuffer from camera
    bool BuildFramebuffer(DisplayEntry& entry);

    KL_BEGIN_FIELDS(DisplayManager)
        KL_FIELD(DisplayManager, nextDisplayId_, "Next display id", 0, 2147483647),
        KL_FIELD(DisplayManager, vsyncEnabled_, "Vsync enabled", 0, 1),
        KL_FIELD(DisplayManager, autoPresent_, "Auto present", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DisplayManager)
        KL_METHOD_AUTO(DisplayManager, AddDisplayRaw, "Add display"),
        KL_METHOD_AUTO(DisplayManager, RemoveDisplay, "Remove display"),
        KL_METHOD_AUTO(DisplayManager, GetDisplay, "Get display"),
        KL_METHOD_AUTO(DisplayManager, GetDisplayCount, "Get display count"),
        KL_METHOD_AUTO(DisplayManager, GetDisplayIds, "Get display ids"),
        KL_METHOD_AUTO(DisplayManager, RouteCamera, "Route camera"),
        KL_METHOD_AUTO(DisplayManager, UnrouteCamera, "Unroute camera"),
        KL_METHOD_AUTO(DisplayManager, GetCameraDisplay, "Get camera display"),
        KL_METHOD_AUTO(DisplayManager, GetDisplayCamera, "Get display camera"),
        KL_METHOD_AUTO(DisplayManager, PresentAll, "Present all"),
        KL_METHOD_AUTO(DisplayManager, Present, "Present"),
        KL_METHOD_AUTO(DisplayManager, ClearAll, "Clear all"),
        KL_METHOD_AUTO(DisplayManager, Clear, "Clear"),
        KL_METHOD_AUTO(DisplayManager, SetVSyncEnabled, "Set vsync enabled"),
        KL_METHOD_AUTO(DisplayManager, SetAutoPresent, "Set auto present"),
        KL_METHOD_AUTO(DisplayManager, IsVSyncEnabled, "Is vsync enabled"),
        KL_METHOD_AUTO(DisplayManager, IsAutoPresentEnabled, "Is auto present enabled")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DisplayManager)
        KL_CTOR0(DisplayManager)
    KL_END_DESCRIBE(DisplayManager)
};

} // namespace koilo
