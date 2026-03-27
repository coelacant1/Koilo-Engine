// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_backend_factory.hpp
 * @brief Factory for creating the best available render backend.
 *
 * Lives in the display library (links OpenGL). Applications that link
 * koiloengine_display can call CreateBestRenderBackend() to get a GPU
 * render backend when an OpenGL context is available, falling back to
 * the software rasterizer otherwise.
 *
 * @date 04/03/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/render/irenderbackend.hpp>
#include <memory>

namespace koilo {

class OpenGLBackend;

/**
 * @brief Create the best available render backend.
 *
 * If an OpenGL 3.3+ context is current, creates an OpenGLRenderBackend.
 * Otherwise, falls back to SoftwareRenderBackend.
 *
 * @return Initialized render backend (never null).
 */
std::unique_ptr<IRenderBackend> CreateBestRenderBackend();

/**
 * @brief Try to create the GPU render backend (legacy path, no display pointer).
 */
std::unique_ptr<IRenderBackend> TryCreateGPURenderBackend();

/**
 * @brief Try to create an OpenGL render backend.
 *
 * Uses unified RHI pipeline (OpenGLRHIDevice + RenderPipeline) by default.
 * Falls back to legacy OpenGLRenderBackend if r.legacy_backend is set or
 * if the unified pipeline fails.
 *
 * @param display Pointer to an initialized OpenGLBackend display.
 * @return Initialized render backend, or nullptr on failure.
 */
std::unique_ptr<IRenderBackend> TryCreateOpenGLRenderBackend(OpenGLBackend* display);

/**
 * @brief Create a software render backend with KSL CPU shading.
 */
std::unique_ptr<IRenderBackend> CreateBestSoftwareBackend();

#ifdef KL_HAVE_VULKAN_BACKEND
class VulkanBackend;

/**
 * @brief Create a Vulkan GPU render backend.
 *
 * @param display Pointer to an initialized VulkanBackend display.
 * @return VulkanRenderBackend if init succeeds, nullptr otherwise.
 */
std::unique_ptr<IRenderBackend> TryCreateVulkanRenderBackend(VulkanBackend* display);
#endif

} // namespace koilo
