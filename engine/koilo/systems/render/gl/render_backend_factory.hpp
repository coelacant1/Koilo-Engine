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
 * @brief Try to create the GPU render backend.
 *
 * @return OpenGLRenderBackend if GL context available and init succeeds,
 *         nullptr otherwise.
 */
std::unique_ptr<IRenderBackend> TryCreateGPURenderBackend();

/**
 * @brief Create a software render backend with KSL CPU shading.
 *
 * Initializes the KSL registry for CPU-only rendering and sets it
 * on KSLMaterial for deferred resolution.
 *
 * @return Initialized software render backend (never null).
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
