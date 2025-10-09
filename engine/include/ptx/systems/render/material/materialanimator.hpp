// materialanimator.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../material/imaterial.hpp"
#include "../../scene/animation/easyeaseanimator.hpp"
#include "../../../core/color/rgbcolor.hpp"
#include "../../../registry/reflect_macros.hpp"

class MaterialAnimator;

/**
 * @file materialanimator.hpp
 * @brief Animated opacity blender using runtime-managed layer storage.
 */
class MaterialAnimatorShader final : public IShader {
public:
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override;

    PTX_BEGIN_FIELDS(MaterialAnimatorShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MaterialAnimatorShader)
        PTX_METHOD_AUTO(MaterialAnimatorShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MaterialAnimatorShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(MaterialAnimatorShader)

};

/**
 * @brief Empty parameter block placeholder for MaterialAnimator.
 */
struct MaterialAnimatorParams {

    PTX_BEGIN_FIELDS(MaterialAnimatorParams)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MaterialAnimatorParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MaterialAnimatorParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(MaterialAnimatorParams)

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

    MaterialAnimator(std::nullptr_t);

    void SetBaseMaterial(Method method, IMaterial* material);

    void AddMaterial(Method method,
                     IMaterial* material,
                     uint16_t frames,
                     float minOpacity,
                     float maxOpacity);

    void AddMaterialFrame(IMaterial& material, float opacity);

    float GetMaterialOpacity(IMaterial& material) const;

    using IMaterial::Update;
    void Update();

    std::size_t GetCapacity() const noexcept { return capacity_; }
    std::size_t GetActiveLayerCount() const noexcept { return currentLayers_; }

private:
    struct Layer {
        Method method = Method::Bypass;
        IMaterial* material = nullptr;

        PTX_BEGIN_FIELDS(Layer)
            PTX_FIELD(Layer, method, "Method", 0, 0),
            PTX_FIELD(Layer, material, "Material", 0, 0)
        PTX_END_FIELDS

        PTX_BEGIN_METHODS(Layer)
            /* No reflected methods. */
        PTX_END_METHODS

        PTX_BEGIN_DESCRIBE(Layer)
            /* No reflected ctors. */
        PTX_END_DESCRIBE(Layer)

    };

    static const IShader* ShaderPtr();

    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::size_t FindLayerIndex(const IMaterial* material) const;
    std::size_t FindLayerIndex(const IMaterial& material) const;

    friend class MaterialAnimatorShader;

    std::size_t capacity_;
    std::size_t currentLayers_ = 0;
    bool baseMaterialSet_ = false;

    EasyEaseAnimator animator_;
    std::vector<Layer> layers_;
    std::vector<float> materialRatios_;
    std::vector<float> opacities_;

    PTX_BEGIN_FIELDS(MaterialAnimator)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MaterialAnimator)
        PTX_METHOD_AUTO(MaterialAnimator, SetBaseMaterial, "Set base material"),
        PTX_METHOD_AUTO(MaterialAnimator, AddMaterial, "Add material"),
        PTX_METHOD_AUTO(MaterialAnimator, AddMaterialFrame, "Add material frame"),
        PTX_METHOD_AUTO(MaterialAnimator, GetMaterialOpacity, "Get material opacity"),
    ptx::make::MakeMethod<MaterialAnimator, static_cast<void (MaterialAnimator::*)()>(&MaterialAnimator::Update)>("Update", "Update"),
        PTX_METHOD_AUTO(MaterialAnimator, GetCapacity, "Get capacity"),
        PTX_METHOD_AUTO(MaterialAnimator, GetActiveLayerCount, "Get active layer count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MaterialAnimator)
        PTX_CTOR(MaterialAnimator, std::size_t, IEasyEaseAnimator::InterpolationMethod),
        PTX_CTOR(MaterialAnimator, std::nullptr_t)
    PTX_END_DESCRIBE(MaterialAnimator)

};
