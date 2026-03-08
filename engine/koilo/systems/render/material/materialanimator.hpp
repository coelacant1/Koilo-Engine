// SPDX-License-Identifier: GPL-3.0-or-later
// materialanimator.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/systems/scene/animation/easyeaseanimator.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

class MaterialAnimator;

/**
 * @file materialanimator.hpp
 * @brief Animated opacity blender using runtime-managed layer storage.
 */
class MaterialAnimatorShader final : public IShader {
public:
    Color888 Shade(const SurfaceProperties& sp, const IMaterial& m) const override;

    KL_BEGIN_FIELDS(MaterialAnimatorShader)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MaterialAnimatorShader)
        KL_METHOD_AUTO(MaterialAnimatorShader, Shade, "Shade")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MaterialAnimatorShader)
        /* No reflected ctors. */
    KL_END_DESCRIBE(MaterialAnimatorShader)

};

/**
 * @brief Empty parameter block placeholder for MaterialAnimator.
 */
struct MaterialAnimatorParams {

    KL_BEGIN_FIELDS(MaterialAnimatorParams)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MaterialAnimatorParams)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MaterialAnimatorParams)
        /* No reflected ctors. */
    KL_END_DESCRIBE(MaterialAnimatorParams)

};

/**
 * @brief Stateful material that animates layer opacities and blends them at shade time.
 */
class MaterialAnimator : public IMaterial {
public:
    enum class Method : uint8_t {
        Base,
        Add,
        Subtract,
        Multiply,
        Divide,
        Darken,
        Lighten,
        Screen,
        Overlay,
        SoftLight,
        Replace,
        EfficientMask,
        Bypass
    };

    static constexpr std::size_t kDefaultLayerCapacity = 4;

    explicit MaterialAnimator(std::size_t maxLayers = kDefaultLayerCapacity,
                              IEasyEaseAnimator::InterpolationMethod defaultMethod = IEasyEaseAnimator::Cosine);

    void SetBaseMaterial(Method method, IMaterial* material);

    // Script-friendly overload: defaults to Method::Base.
    void SetBaseMaterial(IMaterial* material);

    void AddMaterial(Method method,
                     IMaterial* material,
                     uint16_t frames,
                     float minOpacity,
                     float maxOpacity);

    // Script-friendly overload with defaults.
    void AddMaterial(IMaterial* material, float maxOpacity);

    void AddMaterialFrame(IMaterial* material, float opacity);

    float GetMaterialOpacity(IMaterial* material) const;

    using IMaterial::Update;
    void Update();

    std::size_t GetCapacity() const noexcept { return capacity_; }
    std::size_t GetActiveLayerCount() const noexcept { return currentLayers_; }

private:
    struct Layer {
        Method method = Method::Bypass;
        IMaterial* material = nullptr;

        KL_BEGIN_FIELDS(Layer)
            KL_FIELD(Layer, method, "Method", 0, 0),
            KL_FIELD(Layer, material, "Material", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Layer)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Layer)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Layer)

    };

    static const IShader* ShaderPtr();

    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::size_t FindLayerIndex(const IMaterial* material) const;

    friend class MaterialAnimatorShader;

    std::size_t capacity_;
    std::size_t currentLayers_ = 0;
    bool baseMaterialSet_ = false;

    EasyEaseAnimator animator_;
    std::vector<Layer> layers_;
    std::vector<float> materialRatios_;
    std::vector<float> opacities_;

    KL_BEGIN_FIELDS(MaterialAnimator)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MaterialAnimator)
        koilo::make::MakeMethod<MaterialAnimator, static_cast<void (MaterialAnimator::*)(IMaterial*)>(&MaterialAnimator::SetBaseMaterial)>("SetBaseMaterial", "Set base material"),
        koilo::make::MakeMethod<MaterialAnimator, static_cast<void (MaterialAnimator::*)(IMaterial*, float)>(&MaterialAnimator::AddMaterial)>("AddMaterial", "Add material layer"),
        KL_METHOD_AUTO(MaterialAnimator, AddMaterialFrame, "Add material frame"),
        KL_METHOD_AUTO(MaterialAnimator, GetMaterialOpacity, "Get material opacity"),
        koilo::make::MakeMethod<MaterialAnimator, static_cast<void (MaterialAnimator::*)()>(&MaterialAnimator::Update)>("Update", "Update"),
        KL_METHOD_AUTO(MaterialAnimator, GetCapacity, "Get capacity"),
        KL_METHOD_AUTO(MaterialAnimator, GetActiveLayerCount, "Get active layer count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MaterialAnimator)
        KL_CTOR0(MaterialAnimator),
        KL_CTOR(MaterialAnimator, std::size_t),
        KL_CTOR(MaterialAnimator, std::size_t, IEasyEaseAnimator::InterpolationMethod)
    KL_END_DESCRIBE(MaterialAnimator)

};

} // namespace koilo
