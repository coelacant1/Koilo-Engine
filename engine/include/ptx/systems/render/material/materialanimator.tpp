#pragma once
#include "materialanimator.hpp"

template<std::size_t N>
RGBColor MaterialAnimatorShader<N>::Shade(const SurfaceProperties& sp,
                                          const IMaterial&         m) {
    const auto& anim = static_cast<const MaterialAnimator<N>&>(m);
    // Forward shading to the internal CombineMaterial
    const IMaterial& combMat = anim.GetCombiner();
    return combMat.GetShader()->Shade(sp, combMat);
}

template<std::size_t materialCount>
const IShader* MaterialAnimator<materialCount>::ShaderPtr() {
    static const ShaderT kInstance{};
    return &kInstance;
}

template<std::size_t materialCount>
MaterialAnimator<materialCount>::MaterialAnimator()
: IMaterial(ShaderPtr()) {}

template<std::size_t materialCount>
void MaterialAnimator<materialCount>::SetBaseMaterial(Method method, IMaterial* material) {
    if (!material) return;

    if (baseMaterialSet_) {
        combine_.SetMaterial(0, material);
        combine_.SetMethod(0, method);
        combine_.SetOpacity(0, 1.0f);
        dictionary_[0] = material;
        materialRatios_[0] = 1.0f;
        return;
    }

    // First time: allocate layer 0 and keep opacity fixed at 1.0
    baseMaterialSet_ = true;

    // Add as first layer
    combine_.AddMaterial(method, material, /*opacity*/1.0f);
    dictionary_[0]      = material;
    materialRatios_[0]  = 1.0f;
    ++currentMaterials_;
}

template<std::size_t materialCount>
void MaterialAnimator<materialCount>::AddMaterial(Method     method,
                                                  IMaterial* material,
                                                  uint16_t   frames,
                                                  float      minOpacity,
                                                  float      maxOpacity) {
    if (!material || !baseMaterialSet_) return;
    if (currentMaterials_ >= materialCount) return;

    // Prevent duplicates (except base at index 0 which is already set)
    for (uint8_t i = 1; i < currentMaterials_; ++i) {
        if (dictionary_[i] == material) return;
    }

    // Allocate new layer in combiner with starting opacity = minOpacity
    combine_.AddMaterial(method, material, Mathematics::Constrain(minOpacity, 0.0f, 1.0f));

    // Register an animated track for this layer’s opacity
    const uint8_t idx = currentMaterials_;
    dictionary_[idx]  = material;
    materialRatios_[idx] = minOpacity;

    eEA_.AddParameter(&materialRatios_[idx],
                      idx,           // channel
                      frames,
                      minOpacity,
                      maxOpacity);

    ++currentMaterials_;
}

template<std::size_t materialCount>
void MaterialAnimator<materialCount>::AddMaterialFrame(IMaterial& material, float opacity) {
    for (uint8_t i = 0; i < currentMaterials_; ++i) {
        if (dictionary_[i] == &material) {
            eEA_.AddParameterFrame(i, Mathematics::Constrain(opacity, 0.0f, 1.0f));
            break;
        }
    }
}

template<std::size_t materialCount>
float MaterialAnimator<materialCount>::GetMaterialOpacity(IMaterial& material) const {
    for (uint8_t i = 0; i < currentMaterials_; ++i) {
        if (dictionary_[i] == &material) {
            return eEA_.GetValue(i);
        }
    }
    return 0.0f;
}

template<std::size_t materialCount>
void MaterialAnimator<materialCount>::Update() {
    eEA_.Update();

    // Keep layer 0 at 1.0 (base), animate the rest
    for (uint8_t i = 1; i < currentMaterials_; ++i) {
        combine_.SetOpacity(i, Mathematics::Constrain(materialRatios_[i], 0.0f, 1.0f));
    }

    // Ensure base layer present
    if (baseMaterialSet_) combine_.SetOpacity(0, 1.0f);
}
