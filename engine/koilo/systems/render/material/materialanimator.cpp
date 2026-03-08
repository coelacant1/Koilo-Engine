#include <ptx/systems/render/material/materialanimator.hpp>

#include <algorithm>
#include <cstdint>

#include <ptx/core/math/mathematics.hpp>
#include <ptx/core/math/vector3d.hpp>

RGBColor MaterialAnimatorShader::Shade(const SurfaceProperties& sp, const IMaterial& m) const {
    const auto& animator = static_cast<const MaterialAnimator&>(m);

    Vector3D rgb { 0.0f, 0.0f, 0.0f };
    const std::size_t layerCount = animator.currentLayers_;

    for (std::size_t i = 0; i < layerCount; ++i) {
        const float opacity = animator.opacities_[i];
        if (opacity <= 0.025f) {
            continue;
        }

        const IMaterial* child = animator.layers_[i].material;
        if (!child || !child->GetShader()) {
            continue;
        }

        const RGBColor src = child->GetShader()->Shade(sp, *child);
        Vector3D s { float(src.R), float(src.G), float(src.B) };
        Vector3D t;

        switch (animator.layers_[i].method) {
            case MaterialAnimator::Method::Base: {
                rgb = s * opacity;
            } break;

            case MaterialAnimator::Method::Add: {
                t = Vector3D(rgb.X + s.X, rgb.Y + s.Y, rgb.Z + s.Z);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Subtract: {
                t = Vector3D(rgb.X - s.X, rgb.Y - s.Y, rgb.Z - s.Z);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Multiply: {
                t = Vector3D(rgb.X * s.X, rgb.Y * s.Y, rgb.Z * s.Z);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Divide: {
                auto safeDiv = [](float x, float y) {
                    return y != 0.0f ? (x / y) : x;
                };
                t = Vector3D(safeDiv(rgb.X, s.X), safeDiv(rgb.Y, s.Y), safeDiv(rgb.Z, s.Z));
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Darken: {
                t = Vector3D::Min(s, rgb);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Lighten: {
                t = Vector3D::Max(s, rgb);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Screen: {
                t.X = 255.0f - (255.0f - rgb.X) * (255.0f - s.X) / 255.0f;
                t.Y = 255.0f - (255.0f - rgb.Y) * (255.0f - s.Y) / 255.0f;
                t.Z = 255.0f - (255.0f - rgb.Z) * (255.0f - s.Z) / 255.0f;
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Overlay: {
                auto ov = [](float aC, float bC) {
                    return (aC < 128.0f)
                        ? (2.0f * aC * bC / 255.0f)
                        : (255.0f - 2.0f * (255.0f - aC) * (255.0f - bC) / 255.0f);
                };
                t.X = ov(rgb.X, s.X);
                t.Y = ov(rgb.Y, s.Y);
                t.Z = ov(rgb.Z, s.Z);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::SoftLight: {
                auto sl = [](float aC, float bC) {
                    const float A = aC / 255.0f;
                    const float B = bC / 255.0f;
                    return 255.0f * ((1.0f - 2.0f * B) * A * A + 2.0f * B * A);
                };
                t.X = sl(rgb.X, s.X);
                t.Y = sl(rgb.Y, s.Y);
                t.Z = sl(rgb.Z, s.Z);
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::Replace: {
                t = s;
                rgb = Vector3D::LERP(rgb, t, opacity);
            } break;

            case MaterialAnimator::Method::EfficientMask:
                if (src.R > 128 && src.G > 128 && src.B > 128) {
                    rgb = s * opacity;
                    i = layerCount;
                }
                continue;

            case MaterialAnimator::Method::Bypass:
                // Evaluate child but ignore result.
                continue;

            default:
                break;
        }
    }

    rgb = rgb.Constrain(0.0f, 255.0f);
    return RGBColor(uint8_t(rgb.X), uint8_t(rgb.Y), uint8_t(rgb.Z));
}

const IShader* MaterialAnimator::ShaderPtr() {
    static const MaterialAnimatorShader kInstance{};
    return &kInstance;
}

MaterialAnimator::MaterialAnimator(std::size_t maxLayers,
                                   IEasyEaseAnimator::InterpolationMethod defaultMethod)
    : IMaterial(ShaderPtr()),
      capacity_(std::max<std::size_t>(1, maxLayers)),
      animator_(capacity_, defaultMethod),
      layers_(capacity_),
      materialRatios_(capacity_, 0.0f),
      opacities_(capacity_, 0.0f) {}

MaterialAnimator::MaterialAnimator(std::nullptr_t)
    : MaterialAnimator(kDefaultLayerCapacity) {}

void MaterialAnimator::SetBaseMaterial(Method method, IMaterial* material) {
    if (!material) {
        return;
    }

    layers_[0].method = method;
    layers_[0].material = material;
    materialRatios_[0] = 1.0f;
    opacities_[0] = 1.0f;

    if (!baseMaterialSet_) {
        baseMaterialSet_ = true;
        currentLayers_ = std::max<std::size_t>(currentLayers_, 1);
    }
}

void MaterialAnimator::AddMaterial(Method method,
                                   IMaterial* material,
                                   uint16_t frames,
                                   float minOpacity,
                                   float maxOpacity) {
    if (!material || !baseMaterialSet_ || currentLayers_ >= capacity_) {
        return;
    }

    if (FindLayerIndex(material) != kInvalidIndex) {
        return;
    }

    const std::size_t index = currentLayers_;
    const float minClamped = Mathematics::Constrain(minOpacity, 0.0f, 1.0f);
    const float maxClamped = Mathematics::Constrain(maxOpacity, 0.0f, 1.0f);

    layers_[index].method = method;
    layers_[index].material = material;
    materialRatios_[index] = minClamped;
    opacities_[index] = minClamped;

    animator_.AddParameter(&materialRatios_[index],
                           static_cast<uint16_t>(index),
                           frames,
                           minClamped,
                           maxClamped);

    ++currentLayers_;
}

void MaterialAnimator::AddMaterialFrame(IMaterial& material, float opacity) {
    const std::size_t index = FindLayerIndex(material);
    if (index == kInvalidIndex || index == 0) {
        return;
    }

    animator_.AddParameterFrame(static_cast<uint16_t>(index), Mathematics::Constrain(opacity, 0.0f, 1.0f));
}

float MaterialAnimator::GetMaterialOpacity(IMaterial& material) const {
    const std::size_t index = FindLayerIndex(material);
    if (index == kInvalidIndex) {
        return 0.0f;
    }

    if (index == 0) {
        return baseMaterialSet_ ? 1.0f : 0.0f;
    }

    return animator_.GetValue(static_cast<uint16_t>(index));
}
void MaterialAnimator::Update() {
    if (!baseMaterialSet_) {
        return;
    }

    animator_.Update();

    materialRatios_[0] = 1.0f;
    opacities_[0] = 1.0f;

    for (std::size_t i = 1; i < currentLayers_; ++i) {
        opacities_[i] = Mathematics::Constrain(materialRatios_[i], 0.0f, 1.0f);
    }
}

std::size_t MaterialAnimator::FindLayerIndex(const IMaterial* material) const {
    if (!material) {
        return kInvalidIndex;
    }

    for (std::size_t i = 0; i < currentLayers_; ++i) {
        if (layers_[i].material == material) {
            return i;
        }
    }

    return kInvalidIndex;
}

std::size_t MaterialAnimator::FindLayerIndex(const IMaterial& material) const {
    return FindLayerIndex(&material);
}
