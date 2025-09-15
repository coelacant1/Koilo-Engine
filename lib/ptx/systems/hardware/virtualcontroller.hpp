/**
 * @file virtualcontroller.hpp
 * @brief Publishes camera registry, per-camera RGB framebuffers, geometry, and shared control over SHM IPC.
 * @date 8/18/2025
 *
 * Endpoints:
 *  - Registry:   /ptx_reg        (lists all cameras + metadata)
 *  - Per-camera: /ptx_fb{idx}    (1×N RGB888 stream for colors)
 *               /ptx_geom{idx}  (N XY float32 pairs for pixel positions)
 *  - Control:    /ptx_ctrl       (shared control block)
 *
 * The frontend enumerates /ptx_reg and opens the /ptx_fb{idx} + /ptx_geom{idx} pair for the chosen camera.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "../../registry/reflect_macros.hpp"

#include "../../core/color/rgbcolor.hpp"
#include "../render/core/cameramanager.hpp"
#include "../../platform/ipc/ptx_shm.hpp"
#include "controller.hpp"

/**
 * @class VirtualController
 * @brief Controller that publishes all cameras to shared memory for external UIs/clients.
 *
 * Lifecycle:
 * - @ref Initialize() creates registry and per-camera SHM channels sized to each camera’s pixel group.
 * - @ref Display() samples each camera’s IPixelGroup and publishes XY (geometry) and RGB888 (colors).
 *
 * Topology:
 * - Each camera is published as a 1×N framebuffer (RGB888) and a geometry buffer of N (x,y) pairs.
 * - Pixel order is the IPixelGroup’s native order; no reindexing or reordering is performed.
 */
class VirtualController final : public Controller {
public:
    /**
     * @brief Construct a VirtualController.
     * @param cams      Camera manager providing cameras and pixel groups (not owned).
     * @param ctrl_name SHM control channel name (shared for all cameras), defaults to "/ptx_ctrl".
     * @param reg_name  SHM registry name (enumeration + meta), defaults to "/ptx_reg".
     */
    VirtualController(CameraManager* cams,
                      const char* ctrl_name = "/ptx_ctrl",
                      const char* reg_name  = "/ptx_reg");

    /**
     * @brief Create registry and per-camera SHM objects, sized from current camera set.
     *
     * For each camera:
     * - Allocates RGB888 buffer as 1×N (W=N, H=1).
     * - Allocates geometry buffer with N (x,y) float pairs.
     * - Registers camera in @ref reg_ with logical width/height taken from the pixel group size.
     *
     * Safe to call once per session, or after cameras are (re)configured.
     */
    void Initialize() override;

    /**
     * @brief Publish XY + RGB888 for each camera’s pixel group into its SHM channels.
     *
     * Steps per camera:
     * - Fill staging arrays: XY from IPixelGroup::GetCoordinate(j), RGB from IPixelGroup::GetColor(j).
     * - Publish geometry via ptx_geom_publish().
     * - Publish framebuffer via ptx_publish_rgb888().
     *
     * Cameras with mismatched pixel counts (vs. initial sizing) are skipped.
     */
    void Display() override;

private:
    /**
     * @brief Per-camera SHM state and staging buffers.
     *
     * - @ref shm : Framebuffer (RGB888) and handle to shared control block.
     * - @ref geom: Geometry buffer for N XY pairs.
     * - @ref rgb : Staging RGB888 bytes (size 3*N).
     * - @ref xy  : Staging XY floats (size 2*N).
     * - Names are fixed per index: "/ptx_fb{idx}", "/ptx_geom{idx}", and a human label "Camera{idx}".
     */
    struct PerCam {
        PTXShm  shm;             ///< Color FB (RGB888) + shared control handle
        PTXGeom geom;            ///< Geometry (XY pairs)
        uint32_t count = 0;       ///< Number of pixels (N)
        uint32_t W = 0;           ///< Framebuffer width  (W = N)
        uint32_t H = 1;           ///< Framebuffer height (H = 1)
        std::vector<uint8_t> rgb; ///< RGB888 staging buffer (3*N bytes)
        std::vector<float>   xy;  ///< XY staging buffer (2*N floats)
        std::string fb_name;      ///< SHM name: "/ptx_fb{idx}"
        std::string geom_name;    ///< SHM name: "/ptx_geom{idx}"
        std::string ui_name;      ///< UI label: "Camera{idx}" or custom
    
        PTX_BEGIN_FIELDS(PerCam)
            PTX_FIELD(PerCam, shm, "Shm", 0, 0),
            PTX_FIELD(PerCam, geom, "Geom", 0, 0),
            PTX_FIELD(PerCam, count, "Count", 0, 4294967295),
            PTX_FIELD(PerCam, W, "W", 0, 4294967295),
            PTX_FIELD(PerCam, H, "H", 0, 4294967295),
            PTX_FIELD(PerCam, rgb, "Rgb", -2147483648, 2147483647),
            PTX_FIELD(PerCam, xy, "Xy", -2147483648, 2147483647),
            PTX_FIELD(PerCam, fb_name, "Fb name", 0, 0),
            PTX_FIELD(PerCam, geom_name, "Geom name", 0, 0),
            PTX_FIELD(PerCam, ui_name, "Ui name", 0, 0)
        PTX_END_FIELDS
        
        PTX_BEGIN_METHODS(PerCam)
            /* TODO: PTX_METHOD_AUTO(PerCam, Method, "Doc") */
        PTX_END_METHODS
        
        PTX_BEGIN_DESCRIBE(PerCam)
            /* TODO: PTX_CTOR0(PerCam) or PTX_CTOR(PerCam, ...) */
        PTX_END_DESCRIBE(PerCam)
        
};

    std::string ctrl_name_;  ///< Shared control channel name
    std::string reg_name_;   ///< Registry channel name

    PTXRegistry reg_{};           ///< Registry object (enumeration + camera meta)
    std::vector<PerCam> cams_;     ///< Per-camera SHM/state

    PTX_BEGIN_FIELDS(VirtualController)
        /* TODO: PTX_FIELD(VirtualController, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(VirtualController)
        PTX_METHOD_AUTO(VirtualController, Initialize, "Initialize"),
        PTX_METHOD_AUTO(VirtualController, Display, "Display")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(VirtualController)
        PTX_CTOR(VirtualController, CameraManager *, const char *, const char *)
    PTX_END_DESCRIBE(VirtualController)
    
};
