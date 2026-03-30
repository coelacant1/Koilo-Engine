// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_backend_factory.cpp
 * @brief Factory for creating the best available render backend.
 *
 * Supports three paths:
 *  1. Vulkan unified pipeline (VulkanRHIDevice + RenderPipeline)
 *  2. OpenGL unified pipeline (OpenGLRHIDevice + RenderPipeline)
 *  3. Software unified pipeline (SoftwareRHIDevice + RenderPipeline)
 */

#include <koilo/systems/render/gl/render_backend_factory.hpp>
#ifdef KL_HAVE_OPENGL_BACKEND
#include <koilo/systems/render/rhi/opengl/opengl_rhi_device.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/systems/display/backends/gpu/openglbackend.hpp>
#endif
#include <koilo/systems/render/rhi/software/software_rhi_device.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/systems/render/render_cvars.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/ksl/ksl_module.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <iostream>
#include <fstream>

#ifdef KL_HAVE_VULKAN_BACKEND
#include <koilo/systems/render/rhi/vulkan/vulkan_rhi_device.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#endif

namespace koilo {

#ifdef KL_HAVE_OPENGL_BACKEND

// -- Built-in GLSL sources for the unified OpenGL pipeline ---------------

static const char* s_glSceneVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_cameraPos;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_viewDir;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_position = worldPos.xyz;
    v_normal = normalize(mat3(u_model) * a_normal);
    v_uv = a_uv;
    v_viewDir = normalize(worldPos.xyz - u_cameraPos);
    gl_Position = u_projection * u_view * worldPos;
}
)";

static const char* s_glPinkErrorFragSrc = R"(
#version 330 core
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 0.0, 0.78, 1.0);
}
)";

static const char* s_glLineVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_view;
uniform mat4 u_projection;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_projection * u_view * vec4(a_position, 1.0);
}
)";

static const char* s_glLineFragSrc = R"(
#version 330 core
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";

static const char* s_glOverlayVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";

static const char* s_glOverlayFragSrc = R"(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_texture;
out vec4 FragColor;
void main() {
    FragColor = texture(u_texture, vec2(v_uv.x, 1.0 - v_uv.y));
}
)";

static const char* s_glBlitFragSrc = R"(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_texture;
out vec4 FragColor;
void main() {
    FragColor = texture(u_texture, v_uv);
}
)";

/// @brief Create a unified RenderPipeline backed by an OpenGLRHIDevice.
static std::unique_ptr<IRenderBackend> TryCreateOpenGLRHIPipeline(OpenGLBackend* display) {
    // 1. Create and initialize the RHI device
    auto rhiDevice = std::make_unique<rhi::OpenGLRHIDevice>(display);
    if (!rhiDevice->Initialize()) {
        KL_ERR("RenderBackendFactory", "Failed to initialize OpenGL RHI device");
        return nullptr;
    }

    // 2. Create a KSL registry and scan for GLSL shaders
    auto registry = std::make_unique<ksl::KSLRegistry>();
    ksl::KSLSymbolTable symbols;
    symbols.RegisterAll();

    std::string vertSrc(s_glSceneVertSrc);
    std::vector<std::string> searchPaths = {"shaders", "../shaders", "build/shaders"};
    int kslCount = 0;
    for (const auto& path : searchPaths) {
        kslCount = registry->ScanDirectory(path, vertSrc, &symbols);
        if (kslCount > 0) {
            KL_LOG("RenderBackendFactory", "Loaded %d KSL GLSL shaders from %s/",
                   kslCount, path.c_str());
            break;
        }
    }

    if (kslCount == 0)
        KL_WARN("RenderBackendFactory", "No KSL shaders found for OpenGL RHI pipeline");

    KSLMaterial::SetRegistry(registry.get());

    // 3. Build the shader resolver: resolves GLSL source by shader name
    std::string sceneVert(s_glSceneVertSrc);
    std::string pinkFrag(s_glPinkErrorFragSrc);
    std::string lineVert(s_glLineVertSrc);
    std::string lineFrag(s_glLineFragSrc);
    std::string overlayVert(s_glOverlayVertSrc);
    std::string overlayFrag(s_glOverlayFragSrc);
    std::string blitFrag(s_glBlitFragSrc);

    ksl::KSLRegistry* regPtr = registry.get();
    rhi::ShaderResolver resolver = [regPtr,
                                    sceneVert, pinkFrag,
                                    lineVert, lineFrag,
                                    overlayVert, overlayFrag, blitFrag](
                                       const std::string& name) -> rhi::ShaderData {
        rhi::ShaderData sd;

        if (name == "__blit__") {
            sd.vertexCode     = overlayVert.data();
            sd.vertexCodeSize = overlayVert.size();
            sd.fragCode       = blitFrag.data();
            sd.fragCodeSize   = blitFrag.size();
        } else if (name == "__overlay__") {
            sd.vertexCode     = overlayVert.data();
            sd.vertexCodeSize = overlayVert.size();
            sd.fragCode       = overlayFrag.data();
            sd.fragCodeSize   = overlayFrag.size();
        } else if (name == "__debug_line__") {
            sd.vertexCode     = lineVert.data();
            sd.vertexCodeSize = lineVert.size();
            sd.fragCode       = lineFrag.data();
            sd.fragCodeSize   = lineFrag.size();
        } else if (name == "__pink_error__") {
            sd.vertexCode     = sceneVert.data();
            sd.vertexCodeSize = sceneVert.size();
            sd.fragCode       = pinkFrag.data();
            sd.fragCodeSize   = pinkFrag.size();
        } else {
            // Scene shader: use KSL registry GLSL sources
            ksl::KSLModule* mod = regPtr->GetModule(name);
            if (mod && mod->HasGLSLSource()) {
                const auto& vs = mod->GetGLSLVertexSource();
                const auto& fs = mod->GetGLSLFragmentSource();
                sd.vertexCode     = vs.data();
                sd.vertexCodeSize = vs.size();
                sd.fragCode       = fs.data();
                sd.fragCodeSize   = fs.size();
            } else {
                // Fallback to generic scene vertex + pink error
                sd.vertexCode     = sceneVert.data();
                sd.vertexCodeSize = sceneVert.size();
                sd.fragCode       = pinkFrag.data();
                sd.fragCodeSize   = pinkFrag.size();
            }
        }
        return sd;
    };

    // 4. Configure and initialize the unified pipeline
    auto pipeline = std::make_unique<rhi::RenderPipeline>();

    rhi::RenderPipelineConfig cfg;
    cfg.device           = rhiDevice.get();
    cfg.shaderRegistry   = registry.get();
    cfg.shaderResolver   = std::move(resolver);
    cfg.vulkanDepthRemap = false;     // OpenGL uses [-1,1] depth range
    cfg.initialWidth     = 1920;
    cfg.initialHeight    = 1080;
    pipeline->Configure(cfg);

    pipeline->OwnDevice(std::move(rhiDevice));
    pipeline->OwnRegistry(std::move(registry));

    if (!pipeline->Initialize()) {
        KL_ERR("RenderBackendFactory", "Failed to initialize OpenGL RHI RenderPipeline");
        return nullptr;
    }

    KL_LOG("RenderBackendFactory", "OpenGL render backend active (RHI Pipeline)");
    return pipeline;
}

#endif // KL_HAVE_OPENGL_BACKEND

std::unique_ptr<IRenderBackend> TryCreateOpenGLRenderBackend(OpenGLBackend* display) {
#ifdef KL_HAVE_OPENGL_BACKEND
    if (!display) return nullptr;

    auto pipeline = TryCreateOpenGLRHIPipeline(display);
    if (pipeline) return pipeline;

    KL_ERR("RenderBackendFactory", "OpenGL RHI pipeline failed");
#else
    (void)display;
#endif
    return nullptr;
}

std::unique_ptr<IRenderBackend> CreateBestRenderBackend() {
    // No display pointer available - fall back to software
    return CreateBestSoftwareBackend();
}

// -- Software RHI pipeline -----------------------------------------------

/// @brief Create a unified RenderPipeline backed by a SoftwareRHIDevice.
///
/// The software device stores shader bytecode but doesn't execute it --
/// rasterization is driven by vertex data and pipeline state. The dummy
/// shader resolver provides placeholder bytecode so the pipeline can
/// create shader handles for material binding.
static std::unique_ptr<IRenderBackend> TryCreateSoftwareRHIPipeline() {
    // 1. Create and initialize the RHI device
    auto rhiDevice = std::make_unique<rhi::SoftwareRHIDevice>();
    if (!rhiDevice->Initialize()) {
        KL_ERR("RenderBackendFactory", "Failed to initialize Software RHI device");
        return nullptr;
    }

    // 2. Create a KSL registry and load KSO CPU modules
    auto registry = std::make_unique<ksl::KSLRegistry>();
    ksl::KSLSymbolTable symbols;
    symbols.RegisterAll();

    std::vector<std::string> searchPaths = {"shaders", "../shaders", "build/shaders"};
    int ksoCount = 0;
    for (const auto& path : searchPaths) {
        ksoCount = registry->ScanDirectory(path, "", &symbols);
        if (ksoCount > 0) {
            KL_LOG("RenderBackendFactory", "Loaded %d KSO CPU modules from %s/",
                   ksoCount, path.c_str());
            break;
        }
    }

    KSLMaterial::SetRegistry(registry.get());

    // 3. Build a stub shader resolver: returns 4-byte placeholder bytecode
    //    so RenderPipeline can create valid shader handles.
    static const uint8_t s_stubBytecode[4] = {0};

    rhi::ShaderResolver resolver = [](const std::string& /*name*/) -> rhi::ShaderData {
        rhi::ShaderData sd;
        sd.vertexCode     = s_stubBytecode;
        sd.vertexCodeSize = sizeof(s_stubBytecode);
        sd.fragCode       = s_stubBytecode;
        sd.fragCodeSize   = sizeof(s_stubBytecode);
        return sd;
    };

    // 4. Configure and initialize the unified pipeline
    auto pipeline = std::make_unique<rhi::RenderPipeline>();

    rhi::RenderPipelineConfig cfg;
    cfg.device          = rhiDevice.get();
    cfg.shaderRegistry  = registry.get();
    cfg.shaderResolver  = std::move(resolver);
    cfg.vulkanDepthRemap = false;  // Software uses [-1,1] depth (like OpenGL)
    cfg.initialWidth    = 1920;
    cfg.initialHeight   = 1080;
    pipeline->Configure(cfg);

    pipeline->OwnDevice(std::move(rhiDevice));
    pipeline->OwnRegistry(std::move(registry));

    if (!pipeline->Initialize()) {
        KL_ERR("RenderBackendFactory", "Failed to initialize Software RHI RenderPipeline");
        return nullptr;
    }

    KL_LOG("RenderBackendFactory", "Software render backend active (RHI Pipeline)");
    return pipeline;
}

std::unique_ptr<IRenderBackend> CreateBestSoftwareBackend() {
    auto pipeline = TryCreateSoftwareRHIPipeline();
    if (pipeline) return pipeline;

    KL_ERR("RenderBackendFactory", "Failed to create software RHI pipeline");
    return nullptr;
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

    auto pipeline = TryCreateVulkanRHIPipeline(display);
    if (pipeline) return pipeline;

    KL_ERR("RenderBackendFactory", "Vulkan RHI pipeline failed");
    return nullptr;
}
#endif

} // namespace koilo
