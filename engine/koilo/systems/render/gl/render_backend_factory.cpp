// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_backend_factory.cpp
 * @brief Factory for creating the best available render backend.
 */

#include <koilo/systems/render/gl/render_backend_factory.hpp>
#include <koilo/systems/render/gl/opengl_render_backend.hpp>
#include <koilo/systems/render/gl/software_render_backend.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <iostream>

namespace koilo {

// KSL registry for CPU-only (software) rendering
static ksl::KSLRegistry s_cpuOnlyRegistry;

std::unique_ptr<IRenderBackend> TryCreateGPURenderBackend() {
    auto gpu = std::make_unique<OpenGLRenderBackend>();
    if (gpu->Initialize()) {
        std::cout << "[RenderBackendFactory] GPU render backend active ("
                  << gpu->GetName() << ")" << std::endl;
        return gpu;
    }
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
            std::cout << "[RenderBackendFactory] Loaded " << loaded
                      << " KSL CPU shaders from " << path << "/" << std::endl;
            break;
        }
    }
    KSLMaterial::SetRegistry(&s_cpuOnlyRegistry);

    std::cout << "[RenderBackendFactory] Software render backend active ("
              << sw->GetName() << ")" << std::endl;
    return sw;
}

} // namespace koilo
