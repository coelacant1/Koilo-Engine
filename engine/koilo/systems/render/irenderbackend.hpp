// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file irenderbackend.hpp
 * @brief Abstract interface for rendering backends.
 *
 * Separates scene rendering from display output. Implementations can target:
 * - Software rasterizer (CPU, existing Rasterizer class)
 * - OpenGL 3.3+ (GPU accelerated)
 * - Vulkan (future)
 * - Ray tracer (future)
 *
 * The render backend produces pixel data from a Scene + Camera.
 * The display backend (IDisplayBackend) presents that data to hardware.
 *
 * @date 23/02/2026
 * @author Coela
 */

#pragma once

#include <koilo/core/color/color888.hpp>

namespace koilo {

class Scene;
class CameraBase;

/**
 * @class IRenderBackend
 * @brief Abstract interface for rendering a scene to pixel data.
 *
 * Lifecycle:
 * 1. Initialize()  allocate GPU resources, compile shaders, etc.
 * 2. Per frame: Render(scene, camera)  produce pixels
 * 3. ReadPixels()  copy rendered result to CPU buffer (for post-FX, display)
 * 4. Shutdown()  release resources
 */
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Initialize backend resources. Returns true on success.
    virtual bool Initialize() = 0;

    // Release backend resources.
    virtual void Shutdown() = 0;

    // Check if backend is initialized and ready.
    virtual bool IsInitialized() const = 0;

    /**
     * @brief Render a scene from a camera's perspective.
     * @param scene Scene containing meshes to render.
     * @param camera Camera defining viewpoint and pixel layout.
     *
     * After this call, rendered pixels are available via ReadPixels()
     * or (for software backends) directly in the camera's PixelGroup.
     */
    virtual void Render(Scene* scene, CameraBase* camera) = 0;

    /**
     * @brief Copy rendered pixels to a CPU buffer.
     * @param buffer Output buffer (Color888 array, width×height).
     * @param width Buffer width in pixels.
     * @param height Buffer height in pixels.
     *
     * For software backends this copies from PixelGroup.
     * For GPU backends this performs a GPU->CPU readback.
     */
    virtual void ReadPixels(Color888* buffer, int width, int height) = 0;

    // Human-readable backend name (e.g., "Software Rasterizer", "OpenGL 3.3")
    virtual const char* GetName() const = 0;
};

} // namespace koilo
