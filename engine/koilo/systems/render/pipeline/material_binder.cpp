// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file material_binder.cpp
 * @brief Shared material -> pipeline / UBO binder - implementation.
 *
 * Handles pipeline resolution via the injected PipelineResolver callback
 * and marshals KSL material parameters into std140-layout uniform buffers.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#include "material_binder.hpp"
#include "../rhi/rhi_device.hpp"
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/ksl/ksl_module.hpp>
#include <koilo/ksl/ksl_shader.hpp>
#include <cstring>
#include <vector>

namespace koilo::rhi {

// -- Lifetime -----------------------------------------------------------

MaterialBinder::MaterialBinder(IRHIDevice* device)
    : device_(device) {}

MaterialBinder::~MaterialBinder() { Clear(); }

// -- Setup --------------------------------------------------------------

void MaterialBinder::SetPipelineResolver(PipelineResolver resolver)
{
    resolver_ = std::move(resolver);
}

// -- Binding ------------------------------------------------------------

const MaterialBinding* MaterialBinder::Bind(const KSLMaterial* material)
{
    if (!material || !resolver_)
        return nullptr;

    const uintptr_t key = reinterpret_cast<uintptr_t>(material);

    // Fast path - already cached and valid.
    auto it = cache_.find(key);
    if (it != cache_.end() && it->second.valid)
        return &it->second;

    // Resolve the module / shader name.
    ksl::KSLModule* mod = material->GetModule();
    if (!mod)
        return nullptr;

    const std::string& shaderName = mod->Name();
    if (shaderName.empty())
        return nullptr;

    // Build render flags from shader metadata (if any).
    const RenderFlags* flagsPtr = nullptr;
    RenderFlags flags{};
    if (mod->GetMetadata().count > 0) {
        flags.depthStencil.depthTest  = mod->GetMetaInt("depthTest", 1) != 0;
        flags.depthStencil.depthWrite = mod->GetMetaInt("depthWrite", 1) != 0;
        int cm = mod->GetMetaInt("cullMode", 2); // default Back=2
        flags.cullMode = static_cast<RHICullMode>(
            cm >= 0 && cm <= 3 ? cm : 2);
        flagsPtr = &flags;
    }

    // Ask the backend to compile / look-up the pipeline.
    RHIPipeline pipeline = resolver_(shaderName, flagsPtr);
    if (!pipeline.IsValid())
        return nullptr;

    MaterialBinding& binding = cache_[key];
    binding.pipeline = pipeline;

    // Calculate UBO size from the module's parameter declarations.
    ksl::ParamList params = mod->GetParams();
    size_t uboSize = CalculateUBOLayout(params);

    // Create the uniform buffer if the shader has parameters.
    if (uboSize > 0) {
        RHIBufferDesc desc{};
        desc.size        = uboSize;
        desc.usage       = RHIBufferUsage::Uniform;
        desc.hostVisible = true;

        binding.uniformBuffer = device_->CreateBuffer(desc);
        binding.uniformSize   = uboSize;

        // Marshal current parameter values and upload.
        std::vector<uint8_t> staging(uboSize, 0);
        MarshalUBOData(material, staging.data(), uboSize);
        device_->UpdateBuffer(binding.uniformBuffer,
                              staging.data(), uboSize, 0);
    }

    binding.valid = true;
    return &binding;
}

// -- Uniform update -----------------------------------------------------

void MaterialBinder::UpdateUniforms(const KSLMaterial* material)
{
    if (!material)
        return;

    const uintptr_t key = reinterpret_cast<uintptr_t>(material);
    auto it = cache_.find(key);
    if (it == cache_.end() || !it->second.valid)
        return;

    MaterialBinding& binding = it->second;
    if (binding.uniformSize == 0)
        return;

    std::vector<uint8_t> staging(binding.uniformSize, 0);
    MarshalUBOData(material, staging.data(), binding.uniformSize);
    device_->UpdateBuffer(binding.uniformBuffer,
                          staging.data(), binding.uniformSize, 0);
}

// -- Invalidation -------------------------------------------------------

void MaterialBinder::Invalidate(const KSLMaterial* material)
{
    if (!material)
        return;

    const uintptr_t key = reinterpret_cast<uintptr_t>(material);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        if (it->second.uniformSize > 0)
            device_->DestroyBuffer(it->second.uniformBuffer);
        // Pipeline is NOT destroyed - it may be shared via the resolver cache.
        cache_.erase(it);
    }
}

void MaterialBinder::Clear()
{
    for (auto& [key, binding] : cache_) {
        if (binding.uniformSize > 0)
            device_->DestroyBuffer(binding.uniformBuffer);
        // Pipeline is NOT destroyed - it may be shared via the resolver cache.
    }
    cache_.clear();
}

// -- Accessors ----------------------------------------------------------

size_t MaterialBinder::Size() const { return cache_.size(); }

// -- Std140 layout helpers ----------------------------------------------

size_t MaterialBinder::Std140Align(size_t offset, size_t alignment)
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

size_t MaterialBinder::Std140ParamSize(ksl::ParamType type)
{
    switch (type) {
        case ksl::ParamType::Float: return 4;
        case ksl::ParamType::Int:   return 4;
        case ksl::ParamType::Bool:  return 4;
        case ksl::ParamType::Vec2:  return 8;
        case ksl::ParamType::Vec3:  return 12;
        case ksl::ParamType::Vec4:  return 16;
        default:                    return 4;
    }
}

size_t MaterialBinder::Std140ParamAlignment(ksl::ParamType type)
{
    switch (type) {
        case ksl::ParamType::Float: return 4;
        case ksl::ParamType::Int:   return 4;
        case ksl::ParamType::Bool:  return 4;
        case ksl::ParamType::Vec2:  return 8;
        case ksl::ParamType::Vec3:  return 16; // vec3 has 16-byte alignment in std140
        case ksl::ParamType::Vec4:  return 16;
        default:                    return 4;
    }
}

size_t MaterialBinder::CalculateUBOLayout(const ksl::ParamList& params) const
{
    if (!params.decls || params.count <= 0)
        return 0;

    size_t offset = 0;
    for (int i = 0; i < params.count; ++i) {
        const ksl::ParamDecl& p = params.decls[i];
        size_t align = Std140ParamAlignment(p.type);
        size_t size  = Std140ParamSize(p.type);
        offset = Std140Align(offset, align);

        if (p.flags == ksl::ParamFlags::Array && p.arraySize > 0) {
            size_t stride = Std140Align(size, align);
            offset += static_cast<size_t>(p.arraySize) * stride;
        } else {
            offset += size;
        }
    }
    return offset;
}

// -- UBO marshalling ----------------------------------------------------

void MaterialBinder::MarshalUBOData(const KSLMaterial* material,
                                    void* dst, size_t bufferSize) const
{
    ksl::KSLModule* mod = material->GetModule();
    if (!mod)
        return;

    ksl::ParamList params = mod->GetParams();
    void* instance = material->GetInstance();
    if (!instance || !params.decls || params.count <= 0)
        return;

    uint8_t* out = static_cast<uint8_t*>(dst);
    std::memset(out, 0, bufferSize);
    size_t offset = 0;

    for (int i = 0; i < params.count; ++i) {
        const ksl::ParamDecl& p = params.decls[i];
        size_t align = Std140ParamAlignment(p.type);
        size_t size  = Std140ParamSize(p.type);
        offset = Std140Align(offset, align);

        const uint8_t* src =
            static_cast<const uint8_t*>(instance) + p.offset;

        if (p.flags == ksl::ParamFlags::Array && p.arraySize > 0) {
            size_t stride = Std140Align(size, align);
            for (int j = 0; j < p.arraySize; ++j) {
                size_t elemOffset = Std140Align(offset + static_cast<size_t>(j) * stride, align);
                if (elemOffset + size <= bufferSize)
                    std::memcpy(out + elemOffset, src + static_cast<size_t>(j) * size, size);
            }
            offset += static_cast<size_t>(p.arraySize) * stride;
        } else {
            if (offset + size <= bufferSize)
                std::memcpy(out + offset, src, size);
            offset += size;
        }
    }
}

} // namespace koilo::rhi
