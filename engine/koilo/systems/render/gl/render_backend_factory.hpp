// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_backend_factory.hpp
 * @brief Factory for creating the best available render backend.
 *
 * Applications that link koiloengine_display can call
 * TryCreateOpenGLRenderBackend() or TryCreateVulkanRenderBackend()
 * to get a GPU render backend using the RHI pipeline, falling back
 * to the software rasterizer via CreateBestSoftwareBackend().
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
 * @brief Create a software render backend (no GPU required).
 *
 * @return Initialized software render backend (never null).
 */
std::unique_ptr<IRenderBackend> CreateBestRenderBackend();

/**
 * @brief Try to create an OpenGL render backend via the RHI pipeline.
 *
 * Uses OpenGLRHIDevice + RenderPipeline for unified rendering.
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
 * @brief Create a Vulkan GPU render backend via the RHI pipeline.
 *
 * Uses VulkanRHIDevice + RenderPipeline for unified rendering.
 *
 * @param display Pointer to an initialized VulkanBackend display.
 * @return Initialized render backend, or nullptr on failure.
 */
std::unique_ptr<IRenderBackend> TryCreateVulkanRenderBackend(VulkanBackend* display);
#endif

} // namespace koilo
