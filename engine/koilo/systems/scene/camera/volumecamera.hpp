// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file volumecamera.hpp
 * @brief Camera subclass for off-screen rendering into arbitrary pixel groups.
 *
 * VolumeCamera renders a scene from its transform viewpoint into a small
 * off-screen framebuffer, then samples that framebuffer at each pixel's
 * 2D coordinate to produce a packed RGB output buffer. This is useful for
 * driving LED strips, projectors, or any output that maps scene content
 * onto a non-rectangular pixel arrangement.
 *
 * VolumeCamera is always available on every platform. It has no knowledge
 * of how the output buffer is consumed (SPI, USB, preview window, etc.).
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#include "camerabase.hpp"
#include <koilo/systems/render/core/ipixelgroup.hpp>
#include <koilo/registry/reflect_macros.hpp>

#include <cstdint>
#include <vector>

namespace koilo {

/**
 * @class VolumeCamera
 * @brief Renders a scene into an FBO and samples it at arbitrary 2D pixel positions.
 *
 * Each VolumeCamera owns a reference to an IPixelGroup that defines the 2D
 * sampling positions (normalized 0-1 UV coordinates). The camera is placed in
 * the scene via its Transform, renders from that viewpoint, and produces a
 * tightly packed RGB888 output buffer that can be forwarded to any consumer.
 */
class VolumeCamera : public CameraBase {
private:
    IPixelGroup* pixelGroup_ = nullptr;   ///< Sampling positions (not owned).
    Transform ownedTransform_;            ///< Camera-owned transform for standalone use.
    uint32_t fboWidth_ = 128;             ///< Off-screen FBO width.
    uint32_t fboHeight_ = 128;            ///< Off-screen FBO height.
    float gamma_ = 2.2f;                  ///< Gamma correction exponent.
    uint8_t brightness_ = 255;            ///< Global brightness scale (0-255).
    std::vector<uint8_t> packedOutput_;   ///< Packed RGB888 output (3 bytes per pixel).

    Vector2D maxC_{-1e30f, -1e30f};       ///< Cached maximum coordinate.
    Vector2D minC_{1e30f, 1e30f};         ///< Cached minimum coordinate.
    bool calculatedMax_ = false;          ///< True if maxC_ is valid.
    bool calculatedMin_ = false;          ///< True if minC_ is valid.
    bool useNearest_ = false;             ///< True for nearest-neighbor sampling.

    /// Precomputed gamma LUT (256 entries, computed once on SetGamma).
    uint8_t gammaLut_[256];

    /**
     * @brief Rebuild the gamma lookup table.
     */
    void BuildGammaLut();

    /**
     * @brief Bilinear sample of an RGB888 framebuffer at floating-point UV.
     */
    static void SampleBilinear(const uint8_t* fboData,
                                uint32_t fboW, uint32_t fboH,
                                float u, float v,
                                uint8_t& outR, uint8_t& outG, uint8_t& outB);

    /**
     * @brief Nearest-neighbor sample of an RGB888 framebuffer at floating-point UV.
     */
    static void SampleNearest(const uint8_t* fboData,
                               uint32_t fboW, uint32_t fboH,
                               float u, float v,
                               uint8_t& outR, uint8_t& outG, uint8_t& outB);

public:
    /**
     * @brief Construct a VolumeCamera.
     * @param pixelGroup Pixel group defining sampling positions (not owned).
     */
    explicit VolumeCamera(IPixelGroup* pixelGroup);

    /**
     * @brief Construct a VolumeCamera with explicit transform.
     * @param externalTransform Pointer to an externally-owned transform.
     * @param pixelGroup Pixel group defining sampling positions (not owned).
     */
    VolumeCamera(Transform* externalTransform, IPixelGroup* pixelGroup);

    /**
     * @brief Destructor.
     */
    ~VolumeCamera() override = default;

    // === CameraBase overrides ===

    IPixelGroup* GetPixelGroup() override;
    Vector2D GetCameraMinCoordinate() override;
    Vector2D GetCameraMaxCoordinate() override;
    Vector2D GetCameraCenterCoordinate() override;
    Vector3D GetCameraTransformMin() override;
    Vector3D GetCameraTransformMax() override;
    Vector3D GetCameraTransformCenter() override;

    // === VolumeCamera-specific ===

    /**
     * @brief Set the off-screen FBO resolution.
     * @param width FBO width in pixels.
     * @param height FBO height in pixels.
     */
    void SetResolution(uint32_t width, uint32_t height);

    /**
     * @brief Get FBO width.
     * @return Width in pixels.
     */
    uint32_t GetFBOWidth() const { return fboWidth_; }

    /**
     * @brief Get FBO height.
     * @return Height in pixels.
     */
    uint32_t GetFBOHeight() const { return fboHeight_; }

    /**
     * @brief Set gamma correction exponent.
     * @param gamma Gamma value (typically 2.2 for WS2812 LEDs).
     */
    void SetGamma(float gamma);

    /**
     * @brief Get gamma correction exponent.
     * @return Current gamma value.
     */
    float GetGamma() const { return gamma_; }

    /**
     * @brief Set global brightness.
     * @param brightness Brightness scale (0-255, applied after gamma).
     */
    void SetBrightness(uint8_t brightness) { brightness_ = brightness; }

    /// @brief Set filter mode for pixel sampling.
    /// @param nearest If true, use nearest-neighbor; if false, bilinear.
    void SetNearestFilter(bool nearest) { useNearest_ = nearest; }

    /// @brief Check current filter mode.
    bool IsNearestFilter() const { return useNearest_; }

    /**
     * @brief Get global brightness.
     * @return Current brightness value (0-255).
     */
    uint8_t GetBrightness() const { return brightness_; }

    /**
     * @brief Sample an RGB888 framebuffer at each pixel position.
     *
     * For each pixel in the pixel group, samples the FBO at that pixel's
     * 2D coordinate using bilinear interpolation, applies gamma correction
     * and brightness scaling, and writes the result to the packed output.
     *
     * @param fboData Pointer to RGB888 pixel data (fboWidth * fboHeight * 3 bytes).
     * @param fboWidth Width of the source framebuffer.
     * @param fboHeight Height of the source framebuffer.
     */
    void SamplePixels(const uint8_t* fboData, uint32_t fboWidth, uint32_t fboHeight);

    /**
     * @brief Get the packed RGB888 output buffer.
     * @return Pointer to the output data (3 bytes per pixel, pixel count * 3 total).
     *         Returns nullptr if SamplePixels has not been called.
     */
    const uint8_t* GetPackedOutput() const;

    /**
     * @brief Get the size of the packed output in bytes.
     * @return Size in bytes (pixel count * 3), or 0 if no pixel group.
     */
    size_t GetPackedOutputSize() const;

    /**
     * @brief Get the number of pixels this camera samples.
     * @return Pixel count from the pixel group, or 0 if none.
     */
    uint32_t GetPixelCount() const;

    /**
     * @brief Replace the pixel group (invalidates cached bounds).
     * @param pixelGroup New pixel group (not owned).
     */
    void SetPixelGroup(IPixelGroup* pixelGroup);

    KL_BEGIN_FIELDS(VolumeCamera)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(VolumeCamera)
        KL_METHOD_AUTO(VolumeCamera, GetPixelGroup, "Get pixel group"),
        KL_METHOD_AUTO(VolumeCamera, GetCameraMinCoordinate, "Get camera min coordinate"),
        KL_METHOD_AUTO(VolumeCamera, GetCameraMaxCoordinate, "Get camera max coordinate"),
        KL_METHOD_AUTO(VolumeCamera, GetCameraCenterCoordinate, "Get camera center coordinate"),
        KL_METHOD_AUTO(VolumeCamera, GetCameraTransformMin, "Get camera transform min"),
        KL_METHOD_AUTO(VolumeCamera, GetCameraTransformMax, "Get camera transform max"),
        KL_METHOD_AUTO(VolumeCamera, GetCameraTransformCenter, "Get camera transform center"),
        KL_METHOD_AUTO(VolumeCamera, SetResolution, "Set FBO resolution"),
        KL_METHOD_AUTO(VolumeCamera, GetFBOWidth, "Get FBO width"),
        KL_METHOD_AUTO(VolumeCamera, GetFBOHeight, "Get FBO height"),
        KL_METHOD_AUTO(VolumeCamera, SetGamma, "Set gamma"),
        KL_METHOD_AUTO(VolumeCamera, GetGamma, "Get gamma"),
        KL_METHOD_AUTO(VolumeCamera, SetBrightness, "Set brightness"),
        KL_METHOD_AUTO(VolumeCamera, GetBrightness, "Get brightness"),
        KL_METHOD_AUTO(VolumeCamera, GetPixelCount, "Get pixel count"),
        KL_METHOD_AUTO(CameraBase, GetTransform, "Get transform"),
        KL_METHOD_AUTO(CameraBase, SetPerspective, "Set perspective"),
        KL_METHOD_AUTO(CameraBase, IsPerspective, "Is perspective"),
        KL_METHOD_AUTO(CameraBase, SetSkyGradient, "Set sky gradient"),
        KL_METHOD_AUTO(CameraBase, ClearSkyGradient, "Clear sky gradient")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(VolumeCamera)
        KL_CTOR(VolumeCamera, IPixelGroup *),
        KL_CTOR(VolumeCamera, Transform *, IPixelGroup *)
    KL_END_DESCRIBE(VolumeCamera)

};

} // namespace koilo
