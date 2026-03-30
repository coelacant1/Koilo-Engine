// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file igpu_render_backend.hpp
 * @brief Extended render backend interface for GPU-accelerated backends.
 *
 * Adds methods for direct-to-screen blitting and overlay compositing
 * that are shared by both OpenGL and Vulkan GPU backends.
 *
 * @date 03/15/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/render/irenderbackend.hpp>

namespace koilo {

/**
 * @class IGPURenderBackend
 * @brief Extended interface for GPU-accelerated render backends.
 *
 * Adds capabilities beyond basic IRenderBackend:
 * - BlitToScreen: blit render target to the default framebuffer
 * - CompositeCanvasOverlays: alpha-blend Canvas2D overlays
 * - PrepareFrame/FinishFrame: per-frame GPU resource management
 */
class IGPURenderBackend : public IRenderBackend {
public:
    ~IGPURenderBackend() override = default;

    /**
     * @brief Blit the render target to the default framebuffer (screen).
     * @param screenW Screen/window width in pixels.
     * @param screenH Screen/window height in pixels.
     */
    virtual void BlitToScreen(int screenW, int screenH) = 0;

    /**
     * @brief Composite Canvas2D overlays onto the current framebuffer.
     * @param screenW Screen/window width in pixels.
     * @param screenH Screen/window height in pixels.
     */
    virtual void CompositeCanvasOverlays(int screenW, int screenH) = 0;

    /**
     * @brief Prepare GPU state for a new frame.
     *
     * Called before any rendering. Implementations can use this for
     * per-frame resource management (descriptor set rotation, etc.).
     * Default implementation does nothing.
     */
    virtual void PrepareFrame() {}

    /**
     * @brief Finalize GPU state after rendering.
     *
     * Called after all rendering and compositing. Implementations can
     * use this for submission, synchronization, etc.
     * Default implementation does nothing.
     */
    virtual void FinishFrame() {}
};

} // namespace koilo
