// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_backend_factory.cpp
 * @brief Factory for creating the best available render backend.
 *
 * Supports three paths:
 *  1. Vulkan unified pipeline (VulkanRHIDevice + RenderPipeline)
 *  2. Legacy Vulkan backend (VulkanRenderBackend, opt-in via r.legacy_backend)
 *  3. OpenGL GPU backend -> Software fallback
 */

#include <koilo/systems/render/gl/render_backend_factory.hpp>
#ifdef KL_HAVE_OPENGL_BACKEND
#include <koilo/systems/render/gl/opengl_render_backend.hpp>
#endif
#include <koilo/systems/render/gl/software_render_backend.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/systems/render/render_cvars.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <iostream>
#include <fstream>

#ifdef KL_HAVE_VULKAN_BACKEND
#include <koilo/systems/render/vk/vulkan_render_backend.hpp>
#include <koilo/systems/render/rhi/vulkan/vulkan_rhi_device.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#endif

namespace koilo {

// KSL registry for CPU-only (software) rendering
static ksl::KSLRegistry s_cpuOnlyRegistry;

std::unique_ptr<IRenderBackend> TryCreateGPURenderBackend() {
#ifdef KL_HAVE_OPENGL_BACKEND
    auto gpu = std::make_unique<OpenGLRenderBackend>();
    if (gpu->Initialize()) {
        KL_LOG("RenderBackendFactory", "GPU render backend active (%s)",
               gpu->GetName());
        return gpu;
    }
#endif
    return nullptr;
}

std::unique_ptr<IRenderBackend> CreateBestRenderBackend() {
    // Try GPU first (GL backend initializes KSL registry internally)
    auto gpu = TryCreateGPURenderBackend();
    if (gpu) return gpu;

    // Fall back to software
    return CreateBestSoftwareBackend();
}

std::unique_ptr<IRenderBackend> CreateBestSoftwareBackend() {
    auto sw = std::make_unique<SoftwareRenderBackend>();
    sw->Initialize();

    // Load .kso files for CPU shading
    ksl::KSLSymbolTable symbols;
    symbols.RegisterAll();
    std::vector<std::string> searchPaths = {"shaders", "../shaders", "build/shaders"};
    for (const auto& path : searchPaths) {
        int loaded = s_cpuOnlyRegistry.ScanDirectory(path, "", &symbols);
        if (loaded > 0) {
            KL_LOG("RenderBackendFactory", "Loaded %d KSL CPU shaders from %s/",
                   loaded, path.c_str());
            break;
        }
    }
    KSLMaterial::SetRegistry(&s_cpuOnlyRegistry);

    KL_LOG("RenderBackendFactory", "Software render backend active (%s)",
           sw->GetName());
    return sw;
}

#ifdef KL_HAVE_VULKAN_BACKEND

/// @brief Create a unified RenderPipeline backed by a VulkanRHIDevice.
///
/// Creates the RHI device, loads SPIR-V shaders from the KSL registry,
/// wires a ShaderResolver that resolves SPIR-V bytecode by shader name,
/// and returns the pipeline as an IGPURenderBackend.
static std::unique_ptr<IRenderBackend> TryCreateVulkanRHIPipeline(VulkanBackend* display) {
    // 1. Create and initialize the RHI device
    auto rhiDevice = std::make_unique<rhi::VulkanRHIDevice>(display);
    if (!rhiDevice->Initialize()) {
        KL_ERR("RenderBackendFactory", "Failed to initialize Vulkan RHI device");
        return nullptr;
    }

    // 2. Create a KSL registry and scan for SPIR-V + KSO shaders
    auto registry = std::make_unique<ksl::KSLRegistry>();
    ksl::KSLSymbolTable symbols;
    symbols.RegisterAll();

    std::vector<std::string> searchPaths = {
        "build/shaders/spirv",
        "../build/shaders/spirv",
        "shaders/spirv",
    };
    for (const auto& spirvDir : searchPaths) {
        int count = registry->ScanSPIRVDirectory(spirvDir);
        if (count > 0) {
            KL_LOG("RenderBackendFactory", "Loaded %d SPIR-V shaders from %s",
                   count, spirvDir.c_str());
            // Also load KSO CPU modules for material parameter support
            std::string ksoDir = spirvDir;
            auto pos = ksoDir.rfind("/spirv");
            if (pos != std::string::npos) ksoDir = ksoDir.substr(0, pos);
            int ksoCount = registry->ScanDirectory(ksoDir, "", &symbols);
            if (ksoCount > 0)
                KL_LOG("RenderBackendFactory", "Loaded %d KSO CPU modules from %s",
                       ksoCount, ksoDir.c_str());
            break;
        }
    }

    if (!registry->HasSPIRV()) {
        KL_ERR("RenderBackendFactory", "No SPIR-V shaders found for RHI pipeline");
        return nullptr;
    }

    // Make registry available to KSLMaterial for script-based shader binding
    KSLMaterial::SetRegistry(registry.get());

    // 3. Build the shader resolver: resolves SPIR-V by shader name
    //    Capture raw pointer - the pipeline owns the registry via OwnRegistry().
    //
    //    Built-in pipelines use different vertex shaders than the scene pipeline:
    //      __blit__, __overlay__   -> blit.vert.spv + blit.frag.spv
    //      __debug_line__          -> line.vert.spv + line.frag.spv
    //      __pink_error__          -> scene.vert.spv + uniform_color.frag.spv
    //      sky, everything else    -> scene.vert.spv + <name>.frag.spv

    // Load additional vertex SPIR-V files that built-in pipelines need
    auto loadSPIRV = [](const std::string& path) -> std::vector<uint32_t> {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) return {};
        auto size = f.tellg();
        if (size <= 0 || size % 4 != 0) return {};
        f.seekg(0);
        std::vector<uint32_t> data(static_cast<size_t>(size) / 4);
        f.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    };

    // Find the SPIR-V directory that was successfully scanned
    std::string spirvDir;
    for (const auto& dir : searchPaths) {
        if (std::ifstream(dir + "/scene.vert.spv")) { spirvDir = dir; break; }
    }

    auto blitVertSPV = loadSPIRV(spirvDir + "/blit.vert.spv");
    auto blitFragSPV = loadSPIRV(spirvDir + "/blit.frag.spv");
    auto lineVertSPV = loadSPIRV(spirvDir + "/line.vert.spv");
    auto lineFragSPV = loadSPIRV(spirvDir + "/line.frag.spv");

    if (blitVertSPV.empty()) KL_WARN("RenderBackendFactory", "blit.vert.spv not found");
    if (lineVertSPV.empty()) KL_WARN("RenderBackendFactory", "line.vert.spv not found");

    ksl::KSLRegistry* regPtr = registry.get();
    rhi::ShaderResolver resolver = [regPtr,
                                    blitVert = std::move(blitVertSPV),
                                    blitFrag = std::move(blitFragSPV),
                                    lineVert = std::move(lineVertSPV),
                                    lineFrag = std::move(lineFragSPV)](
                                       const std::string& name) -> rhi::ShaderData {
        rhi::ShaderData sd;

        // Select vertex + fragment SPIR-V based on built-in name
        const std::vector<uint32_t>* vertSPV = nullptr;
        const std::vector<uint32_t>* fragSPV = nullptr;

        if (name == "__blit__" || name == "__overlay__") {
            vertSPV = &blitVert;
            fragSPV = &blitFrag;
        } else if (name == "__debug_line__") {
            vertSPV = &lineVert;
            fragSPV = &lineFrag;
        } else if (name == "__pink_error__") {
            // Pink error uses scene vertex shader + uniform_color fragment
            vertSPV = &regPtr->GetVertexSPIRV();
            fragSPV = &regPtr->GetFragmentSPIRV("uniform_color");
        } else {
            // Scene shader: use shared vertex + per-name fragment
            vertSPV = &regPtr->GetVertexSPIRV();
            fragSPV = &regPtr->GetFragmentSPIRV(name);
        }

        if (vertSPV && !vertSPV->empty()) {
            sd.vertexCode     = vertSPV->data();
            sd.vertexCodeSize = vertSPV->size() * sizeof(uint32_t);
        }
        if (fragSPV && !fragSPV->empty()) {
            sd.fragCode     = fragSPV->data();
            sd.fragCodeSize = fragSPV->size() * sizeof(uint32_t);
        }
        return sd;
    };

    // 4. Configure and initialize the unified pipeline
    auto pipeline = std::make_unique<rhi::RenderPipeline>();

    rhi::RenderPipelineConfig cfg;
    cfg.device          = rhiDevice.get();
    cfg.shaderRegistry  = registry.get();
    cfg.shaderResolver  = std::move(resolver);
    cfg.vulkanDepthRemap = true;     // Vulkan uses [0,1] depth range
    cfg.initialWidth    = 1920;
    cfg.initialHeight   = 1080;
    pipeline->Configure(cfg);

    // Transfer ownership of device + registry to the pipeline
    pipeline->OwnDevice(std::move(rhiDevice));
    pipeline->OwnRegistry(std::move(registry));

    if (!pipeline->Initialize()) {
        KL_ERR("RenderBackendFactory", "Failed to initialize RHI RenderPipeline");
        return nullptr;
    }

    KL_LOG("RenderBackendFactory", "Vulkan render backend active (RHI Pipeline)");
    return pipeline;
}

std::unique_ptr<IRenderBackend> TryCreateVulkanRenderBackend(VulkanBackend* display) {
    if (!display) return nullptr;

    // Allow opt-in to legacy backend via CVar
    if (cvar_r_legacy_backend.Get()) {
        auto vk = std::make_unique<VulkanRenderBackend>(display);
        if (vk->Initialize()) {
            KL_LOG("RenderBackendFactory", "Vulkan render backend active (legacy)");
            return vk;
        }
        return nullptr;
    }

    // Default: unified RHI pipeline
    auto pipeline = TryCreateVulkanRHIPipeline(display);
    if (pipeline) return pipeline;

    // Fallback to legacy backend if unified pipeline fails
    KL_WARN("RenderBackendFactory", "RHI pipeline failed, falling back to legacy Vulkan backend");
    auto vk = std::make_unique<VulkanRenderBackend>(display);
    if (vk->Initialize()) {
        KL_LOG("RenderBackendFactory", "Vulkan render backend active (legacy fallback)");
        return vk;
    }
    return nullptr;
}
#endif

} // namespace koilo
