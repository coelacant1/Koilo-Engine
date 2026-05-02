// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/camera/volumecamera.hpp>

#include <koilo/systems/render/core/ipixelgroup.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/core/math/vector3d.hpp>

#include <algorithm>
#include <cmath>

namespace koilo {

// --- Construction -----------------------------------------------------------

VolumeCamera::VolumeCamera(IPixelGroup* pixelGroup)
    : pixelGroup_(pixelGroup) {
    transform = &ownedTransform_;
    cameraLayout = nullptr;
    Set2D(false);
    BuildGammaLut();
}

VolumeCamera::VolumeCamera(Transform* externalTransform, IPixelGroup* pixelGroup)
    : pixelGroup_(pixelGroup) {
    transform = externalTransform ? externalTransform : &ownedTransform_;
    cameraLayout = nullptr;
    Set2D(false);
    BuildGammaLut();
}

// --- CameraBase overrides ---------------------------------------------------

IPixelGroup* VolumeCamera::GetPixelGroup() {
    return pixelGroup_;
}

Vector2D VolumeCamera::GetCameraMinCoordinate() {
    if (!pixelGroup_) return {};

    if (!calculatedMin_) {
        const uint32_t count = pixelGroup_->GetPixelCount();
        for (uint32_t i = 0; i < count; ++i) {
            const Vector2D coord = pixelGroup_->GetCoordinate(i);
            if (coord.X < minC_.X) minC_.X = coord.X;
            if (coord.Y < minC_.Y) minC_.Y = coord.Y;
        }
        calculatedMin_ = true;
    }

    return minC_;
}

Vector2D VolumeCamera::GetCameraMaxCoordinate() {
    if (!pixelGroup_) return {};

    if (!calculatedMax_) {
        const uint32_t count = pixelGroup_->GetPixelCount();
        for (uint32_t i = 0; i < count; ++i) {
            const Vector2D coord = pixelGroup_->GetCoordinate(i);
            if (coord.X > maxC_.X) maxC_.X = coord.X;
            if (coord.Y > maxC_.Y) maxC_.Y = coord.Y;
        }
        calculatedMax_ = true;
    }

    return maxC_;
}

Vector2D VolumeCamera::GetCameraCenterCoordinate() {
    return (GetCameraMinCoordinate() + GetCameraMaxCoordinate()) / 2.0f;
}

Vector3D VolumeCamera::GetCameraTransformMin() {
    Vector2D minV2 = GetCameraMinCoordinate();
    if (!transform) return {};
    return transform->GetRotation().RotateVector(
        Vector3D(minV2.X, minV2.Y, 0) * transform->GetScale()
    ) + transform->GetPosition();
}

Vector3D VolumeCamera::GetCameraTransformMax() {
    Vector2D maxV2 = GetCameraMaxCoordinate();
    if (!transform) return {};
    return transform->GetRotation().RotateVector(
        Vector3D(maxV2.X, maxV2.Y, 0) * transform->GetScale()
    ) + transform->GetPosition();
}

Vector3D VolumeCamera::GetCameraTransformCenter() {
    return (GetCameraTransformMin() + GetCameraTransformMax()) / 2.0f;
}

// --- Resolution and gamma ---------------------------------------------------

void VolumeCamera::SetResolution(uint32_t width, uint32_t height) {
    fboWidth_ = width > 0 ? width : 1;
    fboHeight_ = height > 0 ? height : 1;
}

void VolumeCamera::SetGamma(float gamma) {
    gamma_ = gamma > 0.0f ? gamma : 1.0f;
    BuildGammaLut();
}

void VolumeCamera::BuildGammaLut() {
    for (int i = 0; i < 256; ++i) {
        float normalized = static_cast<float>(i) / 255.0f;
        float corrected = std::pow(normalized, gamma_);
        int scaled = static_cast<int>(corrected * brightness_ + 0.5f);
        gammaLut_[i] = static_cast<uint8_t>(std::min(scaled, 255));
    }
}

// --- Pixel sampling ---------------------------------------------------------

void VolumeCamera::SampleBilinear(const uint8_t* fboData,
                                   uint32_t fboW, uint32_t fboH,
                                   float u, float v,
                                   uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    // Convert UV to pixel coordinates (center of pixel)
    const float px = u * static_cast<float>(fboW) - 0.5f;
    const float py = v * static_cast<float>(fboH) - 0.5f;

    const int x0 = static_cast<int>(std::floor(px));
    const int y0 = static_cast<int>(std::floor(py));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const float fx = px - static_cast<float>(x0);
    const float fy = py - static_cast<float>(y0);

    // Clamp to valid range
    auto clampX = [fboW](int x) -> uint32_t {
        return static_cast<uint32_t>(std::max(0, std::min(x, static_cast<int>(fboW) - 1)));
    };
    auto clampY = [fboH](int y) -> uint32_t {
        return static_cast<uint32_t>(std::max(0, std::min(y, static_cast<int>(fboH) - 1)));
    };

    // Fetch four neighboring pixels
    auto fetch = [fboData, fboW](uint32_t cx, uint32_t cy) -> const uint8_t* {
        return fboData + (cy * fboW + cx) * 3;
    };

    const uint8_t* p00 = fetch(clampX(x0), clampY(y0));
    const uint8_t* p10 = fetch(clampX(x1), clampY(y0));
    const uint8_t* p01 = fetch(clampX(x0), clampY(y1));
    const uint8_t* p11 = fetch(clampX(x1), clampY(y1));

    // Bilinear interpolation per channel
    for (int c = 0; c < 3; ++c) {
        float top = static_cast<float>(p00[c]) * (1.0f - fx) + static_cast<float>(p10[c]) * fx;
        float bot = static_cast<float>(p01[c]) * (1.0f - fx) + static_cast<float>(p11[c]) * fx;
        float val = top * (1.0f - fy) + bot * fy;
        (&outR)[c] = static_cast<uint8_t>(std::min(std::max(val + 0.5f, 0.0f), 255.0f));
    }
}

void VolumeCamera::SampleNearest(const uint8_t* fboData,
                                  uint32_t fboW, uint32_t fboH,
                                  float u, float v,
                                  uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    const uint32_t x = std::min(static_cast<uint32_t>(u * fboW), fboW - 1);
    const uint32_t y = std::min(static_cast<uint32_t>(v * fboH), fboH - 1);
    const uint8_t* p = fboData + (y * fboW + x) * 3;
    outR = p[0];
    outG = p[1];
    outB = p[2];
}

void VolumeCamera::SamplePixels(const uint8_t* fboData,
                                 uint32_t fboWidth,
                                 uint32_t fboHeight) {
    if (!pixelGroup_ || !fboData || fboWidth == 0 || fboHeight == 0) {
        return;
    }

    const uint32_t count = pixelGroup_->GetPixelCount();
    packedOutput_.resize(count * 3);

    // Rebuild LUT if brightness changed since last build
    // (cheap check: LUT entry for 255 should equal brightness)
    if (gammaLut_[255] != brightness_) {
        BuildGammaLut();
    }

    // Fetch the coordinate array once (single virtual call) instead of
    // dispatching GetCoordinate() per pixel through the interface vtable.
    const Vector2D* coords = pixelGroup_->GetCoordinatesArray();

    for (uint32_t i = 0; i < count; ++i) {
        Vector2D coord;
        if (coords) {
            coord = coords[i];
        } else {
            coord = pixelGroup_->GetCoordinate(i);
        }
        const float u = coord.X;
        const float v = coord.Y;

        uint8_t r = 0, g = 0, b = 0;

        // Pixels outside 0-1 range get black
        if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
            if (useNearest_) {
                SampleNearest(fboData, fboWidth, fboHeight, u, v, r, g, b);
            } else {
                SampleBilinear(fboData, fboWidth, fboHeight, u, v, r, g, b);
            }
        }

        // Apply gamma + brightness via LUT
        const size_t base = static_cast<size_t>(i) * 3;
        packedOutput_[base + 0] = gammaLut_[r];
        packedOutput_[base + 1] = gammaLut_[g];
        packedOutput_[base + 2] = gammaLut_[b];
    }
}

const uint8_t* VolumeCamera::GetPackedOutput() const {
    if (packedOutput_.empty()) return nullptr;
    return packedOutput_.data();
}

size_t VolumeCamera::GetPackedOutputSize() const {
    return packedOutput_.size();
}

uint32_t VolumeCamera::GetPixelCount() const {
    return pixelGroup_ ? pixelGroup_->GetPixelCount() : 0;
}

void VolumeCamera::SetPixelGroup(IPixelGroup* pixelGroup) {
    pixelGroup_ = pixelGroup;
    calculatedMin_ = false;
    calculatedMax_ = false;
    minC_ = Vector2D(1e30f, 1e30f);
    maxC_ = Vector2D(-1e30f, -1e30f);
    packedOutput_.clear();
}

} // namespace koilo
