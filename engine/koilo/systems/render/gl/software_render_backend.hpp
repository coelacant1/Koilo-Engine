// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file software_render_backend.hpp
 * @brief Software rasterizer render backend.
 *
 * Wraps the existing Rasterizer static class behind the IRenderBackend interface.
 * This is the default backend for all platforms including MCU targets.
 *
 * @date 23/02/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/render/irenderbackend.hpp>
#include <koilo/systems/render/raster/rasterizer.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <cstring>

namespace koilo {

/**
 * @class SoftwareRenderBackend
 * @brief CPU-based software rasterizer backend.
 *
 * Delegates to the existing static Rasterizer class.
 * Rendered pixels are stored in the camera's PixelGroup and can be
 * read back via ReadPixels().
 */
class SoftwareRenderBackend : public IRenderBackend {
public:
    bool Initialize() override { initialized_ = true; return true; }
    void Shutdown() override { initialized_ = false; }
    bool IsInitialized() const override { return initialized_; }

    void Render(Scene* scene, CameraBase* camera) override {
        lastCamera_ = camera;
        Rasterizer::Rasterize(scene, camera);
    }

    void ReadPixels(Color888* buffer, int width, int height) override {
        if (!lastCamera_ || !buffer) return;
        IPixelGroup* pg = lastCamera_->GetPixelGroup();
        if (!pg) return;

        // Fast path: memcpy when pixel layout matches buffer
        uint32_t pixelCount = pg->GetPixelCount();
        uint32_t total = static_cast<uint32_t>(width * height);
        uint32_t copyCount = pixelCount < total ? pixelCount : total;
        Color888* src = pg->GetColors();
        if (src && copyCount > 0) {
            std::memcpy(buffer, src, copyCount * sizeof(Color888));
        }
    }

    const char* GetName() const override { return "Software Rasterizer"; }

private:
    bool initialized_ = false;
    CameraBase* lastCamera_ = nullptr;

    KL_BEGIN_FIELDS(SoftwareRenderBackend)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(SoftwareRenderBackend)
        KL_METHOD_AUTO(SoftwareRenderBackend, Initialize, "Initialize"),
        KL_METHOD_AUTO(SoftwareRenderBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(SoftwareRenderBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(SoftwareRenderBackend, Render, "Render"),
        KL_METHOD_AUTO(SoftwareRenderBackend, ReadPixels, "Read pixels"),
        KL_METHOD_AUTO(SoftwareRenderBackend, GetName, "Get name")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SoftwareRenderBackend)
        /* No reflected ctors. */
    KL_END_DESCRIBE(SoftwareRenderBackend)

};

} // namespace koilo
