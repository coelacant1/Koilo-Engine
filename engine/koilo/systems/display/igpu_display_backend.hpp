// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file igpu_display_backend.hpp
 * @brief Extended display backend interface for GPU-accelerated window backends.
 *
 * Adds methods for buffer swapping, no-swap presentation, and texture
 * filtering control that are shared by both OpenGL and Vulkan display backends.
 *
 * @date 03/15/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/display/idisplaybackend.hpp>

namespace koilo {

/**
 * @class IGPUDisplayBackend
 * @brief Extended interface for GPU-accelerated display backends.
 *
 * Adds capabilities beyond basic IDisplayBackend:
 * - SwapOnly: swap front/back buffers without uploading new pixel data
 * - PresentNoSwap: upload pixels without swapping (for multi-pass compositing)
 * - SetNearestFiltering: control texture filtering for pixel-art upscaling
 * - ClearDefaultFramebuffer: clear the screen (not the render FBO)
 */
class IGPUDisplayBackend : public IDisplayBackend {
public:
    ~IGPUDisplayBackend() override = default;

    /**
     * @brief Swap front/back buffers without uploading new pixel data.
     *
     * Use after BlitToScreen + overlays are done, to present the composited frame.
     */
    virtual void SwapOnly() = 0;

    /**
     * @brief Upload pixel data without swapping buffers.
     * @param fb Framebuffer containing pixel data.
     * @return True on success.
     *
     * Use for the software render path: upload CPU-rendered pixels,
     * then call SwapOnly() to present.
     */
    virtual bool PresentNoSwap(const Framebuffer& fb) = 0;

    /**
     * @brief Set texture filtering mode.
     * @param nearest True for nearest-neighbor (sharp pixels), false for bilinear.
     *
     * Use nearest for pixel-art / low-res upscaling. Call after Initialize().
     */
    virtual void SetNearestFiltering(bool nearest) = 0;

    /**
     * @brief Prepare the default framebuffer for compositing.
     * @param width Viewport width.
     * @param height Viewport height.
     *
     * Binds the default framebuffer, sets the viewport, and clears it.
     * Call before BlitToScreen / CompositeCanvasOverlays.
     */
    virtual void PrepareDefaultFramebuffer(int width, int height) = 0;
};

} // namespace koilo
