// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file material_binder.hpp
 * @brief Shared material -> pipeline / UBO binder using the RHI abstraction layer.
 *
 * Resolves a KSLMaterial to a backend-agnostic RHIPipeline and creates a
 * std140-layout uniform buffer for the material's parameters.  Both the
 * OpenGL and Vulkan backends delegate here instead of duplicating the
 * material -> pipeline resolution and uniform marshalling logic.
 *
 * The binder itself is shader-language-agnostic: it receives a
 * PipelineResolver callback from the render pipeline that knows how to
 * compile shaders (GLSL for OpenGL, SPIR-V for Vulkan).
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once

#include "../rhi/rhi_types.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "../../../registry/reflect_macros.hpp"

namespace koilo      { class KSLMaterial; }
namespace koilo::rhi { class IRHIDevice; }
namespace ksl        { struct ParamList; enum class ParamType : uint8_t; }

namespace koilo::rhi {

// -- PipelineResolver callback ------------------------------------------
//
// Given a shader name and optional render flags (from shader metadata),
// returns a ready-to-use RHIPipeline.  The render pipeline provides this,
// handling backend-specific shader compilation.
using PipelineResolver = std::function<RHIPipeline(const std::string& shaderName,
                                                    const RenderFlags* flags)>;

// -- MaterialBinding ----------------------------------------------------

struct MaterialBinding {
    RHIPipeline pipeline;            ///< Resolved pipeline for this material's shader.
    RHIBuffer   uniformBuffer;       ///< Material parameter UBO (std140 layout).
    size_t      uniformSize = 0;     ///< Size of the UBO data in bytes.
    bool        valid       = false; ///< True once the pipeline is resolved.
};

// -- MaterialBinder -----------------------------------------------------

/**
 * @class MaterialBinder
 * @brief Backend-agnostic cache that maps KSLMaterials to RHI pipelines and
 *        std140 uniform buffers.
 *
 * Typical usage:
 *   1. Construct with the RHI device pointer.
 *   2. Call SetPipelineResolver() once during render-pipeline setup.
 *   3. For each KSLMaterial encountered during a frame, call Bind() to
 *      obtain (or create) the cached binding.
 *   4. Call UpdateUniforms() when material parameters change (e.g. per-frame).
 */
class MaterialBinder {
public:
    explicit MaterialBinder(IRHIDevice* device);
    ~MaterialBinder();

    MaterialBinder(const MaterialBinder&)            = delete;
    MaterialBinder& operator=(const MaterialBinder&) = delete;

    // -- Setup ----------------------------------------------------------

    /**
     * @brief Set the pipeline resolver callback (called once during setup).
     * @param resolver  Callback that compiles/caches a pipeline for a shader name.
     */
    void SetPipelineResolver(PipelineResolver resolver);

    // -- Binding --------------------------------------------------------

    /**
     * @brief  Resolve a KSLMaterial to its pipeline + UBO, creating or
     *         reusing a cached binding.
     * @param  material  The KSL material to bind (may be nullptr).
     * @return Pointer to the cached binding, or nullptr if the material is
     *         not a KSL material, has no module, or the resolver is not set.
     */
    const MaterialBinding* Bind(const KSLMaterial* material);

    /**
     * @brief  Re-marshal the material's current parameter values into the
     *         cached UBO and upload to the GPU.
     * @param  material  The KSL material whose uniforms have changed.
     */
    void UpdateUniforms(const KSLMaterial* material);

    // -- Invalidation ---------------------------------------------------

    /**
     * @brief Remove a single material from the cache, destroying its UBO.
     * @note  The pipeline is NOT destroyed - it may be shared via the
     *        resolver's own cache.
     */
    void Invalidate(const KSLMaterial* material);

    /**
     * @brief Destroy every cached UBO and clear the cache.
     */
    void Clear();

    /**
     * @brief Number of material bindings currently held in the cache.
     */
    size_t Size() const;

private:
    IRHIDevice*                                  device_;
    PipelineResolver                             resolver_;
    std::unordered_map<uintptr_t, MaterialBinding> cache_;

    // -- Std140 layout helpers ------------------------------------------

    static size_t Std140Align(size_t offset, size_t alignment);
    static size_t Std140ParamSize(ksl::ParamType type);
    static size_t Std140ParamAlignment(ksl::ParamType type);
    size_t CalculateUBOLayout(const ksl::ParamList& params) const;
    void   MarshalUBOData(const KSLMaterial* material,
                          void* dst, size_t bufferSize) const;

    KL_BEGIN_FIELDS(MaterialBinder)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MaterialBinder)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MaterialBinder)
        KL_CTOR(MaterialBinder, IRHIDevice*)
    KL_END_DESCRIBE(MaterialBinder)

};

} // namespace koilo::rhi
