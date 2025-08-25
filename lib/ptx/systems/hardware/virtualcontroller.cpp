#include "virtualcontroller.hpp"
#include <cstdio>

/**
 * @file virtualcontroller.cpp
 * @brief Implementation of VirtualController SHM publishing.
 * @date 8/18/2025
 */

VirtualController::VirtualController(CameraManager* cams,
                                     const char* ctrl_name,
                                     const char* reg_name)
: Controller(cams),
  ctrl_name_(ctrl_name ? ctrl_name : "/ptx_ctrl"),
  reg_name_(reg_name ? reg_name : "/ptx_reg") {}

void VirtualController::Initialize() {
    const uint32_t camCount = (cameras ? cameras->GetCameraCount() : 0);
    cams_.clear();
    cams_.reserve(camCount);

    // Create the registry once with enough capacity
    (void)ptx_registry_init(reg_, reg_name_.c_str(), camCount);

    // Build per-camera streams
    for (uint32_t i = 0; i < camCount; ++i) {
        CameraBase* cam = cameras->GetCameras()[i];
        if (!cam) continue;

        IPixelGroup* pg = cam->GetPixelGroup();
        if (!pg) continue;

        PerCam pc;

        pc.count = pg->GetPixelCount();
        pc.W     = pc.count;  // 1xN buffer for color
        pc.H     = 1;

        // Use the group’s size as logical W/H
        const Vector2D sz = pg->GetSize();
        const uint32_t logicalW = static_cast<uint32_t>(sz.X);
        const uint32_t logicalH = static_cast<uint32_t>(sz.Y);

        char namebuf[32];
        std::snprintf(namebuf, sizeof(namebuf), "Camera%u", i);
        pc.ui_name  = namebuf;
        pc.fb_name  = std::string("/ptx_fb")   + std::to_string(i);
        pc.geom_name= std::string("/ptx_geom") + std::to_string(i);

        // Init FB (1xN) + shared control
        (void)ptx_shm_init(pc.shm, pc.fb_name.c_str(), pc.W, pc.H, ctrl_name_.c_str());

        // Init Geometry: publish N XY pairs
        pc.geom.width  = logicalW;
        pc.geom.height = logicalH;
        (void)ptx_geom_init(pc.geom, pc.geom_name.c_str(), pc.count);

        pc.rgb.resize(size_t(pc.count) * 3u, 0);
        pc.xy .resize(size_t(pc.count) * 2u, 0.f);

        // Record in registry
        ptx_registry_set(reg_, i, pc.ui_name.c_str(), i, pc.count, logicalW, logicalH);

        cams_.push_back(std::move(pc));
    }

    ptx_registry_publish(reg_, (uint32_t)(cams_.size()));
}

void VirtualController::Display() {
    if (!cameras) return;
    const uint32_t n = Mathematics::Min(cameras->GetCameraCount(), (uint8_t)cams_.size());

    for (uint32_t i = 0; i < n; ++i) {
        CameraBase* cam = cameras->GetCameras()[i];
        if (!cam) continue;

        IPixelGroup* pg = cam->GetPixelGroup();
        if (!pg) continue;

        PerCam& pc = cams_[i];
        if (pg->GetPixelCount() != pc.count) continue;

        // Fill staging (XY, RGB)
        for (uint32_t j = 0; j < pc.count; ++j) {
            const Vector2D coord = pg->GetCoordinate(j);
            const RGBColor* col  = pg->GetColor(j);

            pc.xy[2*j + 0] = coord.X;
            pc.xy[2*j + 1] = coord.Y;

            pc.rgb[3*j + 0] = col ? col->R : 0;
            pc.rgb[3*j + 1] = col ? col->G : 0;
            pc.rgb[3*j + 2] = col ? col->B : 0;
        }

        // Publish this camera
        ptx_geom_publish(pc.geom, pc.xy.data(), pc.count);
        ptx_publish_rgb888(pc.shm, pc.W, pc.H, pc.rgb.data());
    }
}
