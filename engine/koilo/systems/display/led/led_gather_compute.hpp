// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_gather_compute.hpp
 * @brief GPU-accelerated pixel gather via compute shader for LED volume output.
 *
 * LEDGatherCompute samples an FBO texture at arbitrary 2D positions using a
 * GL compute shader, producing a packed RGB888 output buffer. This avoids a
 * full glReadPixels of the FBO -- only the sampled LED colors are read back.
 *
 * Requires GL 4.3+ (or GL_ARB_compute_shader). When unavailable, callers
 * fall back to VolumeCamera::SamplePixels() (CPU bilinear path).
 *
 * This class is self-contained: it compiles its own shader and manages its
 * own GL objects. It does not go through the engine RHI.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#ifdef KL_HAVE_OPENGL_BACKEND

#include <cstdint>
#include <vector>

namespace koilo {

/**
 * @class LEDGatherCompute
 * @brief GPU compute shader that samples an FBO at LED pixel positions.
 *
 * Usage:
 *   1. Call Initialize() once (compiles shader, checks capability).
 *   2. Call UploadPositions() when LED layout changes.
 *   3. Each frame: Dispatch(fboTexture, gamma, brightness) then ReadBack().
 *   4. Call Shutdown() when done.
 */
class LEDGatherCompute {
public:
    LEDGatherCompute();
    ~LEDGatherCompute();

    LEDGatherCompute(const LEDGatherCompute&) = delete;
    LEDGatherCompute& operator=(const LEDGatherCompute&) = delete;

    /**
     * @brief Check if compute shaders are available in the current GL context.
     * @return True if GL 4.3+ or GL_ARB_compute_shader is present.
     */
    static bool IsSupported();

    /**
     * @brief Compile the gather shader and allocate GPU resources.
     * @return True if initialization succeeded, false if compute not available.
     */
    bool Initialize();

    /**
     * @brief Release all GL resources.
     */
    void Shutdown();

    /**
     * @brief Upload LED positions (normalized UV coordinates) to the GPU.
     *
     * Call this when the LED layout changes (not every frame).
     *
     * @param positions Flat array of [u0, v0, u1, v1, ...] float pairs.
     * @param pixelCount Number of LED pixels (positions has 2*pixelCount floats).
     */
    void UploadPositions(const float* positions, uint32_t pixelCount);

    /**
     * @brief Run the gather compute shader.
     *
     * Samples the FBO texture at each uploaded LED position, applies gamma
     * correction, and writes packed RGB to the output SSBO.
     *
     * @param fboTextureId GL texture handle for the scene FBO color attachment.
     * @param gamma Gamma correction exponent (typically 2.2).
     * @param brightness Global brightness scale (0.0 - 1.0).
     */
    void Dispatch(uint32_t fboTextureId, float gamma, float brightness);

    /**
     * @brief Read the packed RGB888 output back from the GPU.
     *
     * Blocks until the compute shader finishes (GL fence + map).
     *
     * @param output Destination buffer (must be at least pixelCount * 3 bytes).
     * @return Number of bytes written, or 0 on failure.
     */
    size_t ReadBack(uint8_t* output);

    /**
     * @brief Get the currently uploaded pixel count.
     * @return Pixel count, or 0 if no positions uploaded.
     */
    uint32_t GetPixelCount() const { return pixelCount_; }

    /**
     * @brief Check if the shader is compiled and ready.
     * @return True if Initialize() succeeded and Shutdown() has not been called.
     */
    bool IsReady() const { return ready_; }

    /**
     * @brief Get the embedded GLSL compute shader source.
     * @return Null-terminated GLSL source string.
     */
    static const char* GetShaderSource();

private:
    uint32_t program_ = 0;          ///< GL program handle.
    uint32_t positionsSSBO_ = 0;    ///< SSBO for LED UV positions.
    uint32_t outputSSBO_ = 0;       ///< SSBO for packed RGB output.
    uint32_t pixelCount_ = 0;       ///< Current number of LED pixels.
    int gammaLoc_ = -1;             ///< Uniform location for gamma.
    int brightnessLoc_ = -1;        ///< Uniform location for brightness.
    int pixelCountLoc_ = -1;        ///< Uniform location for pixel count.
    bool ready_ = false;            ///< True if initialized successfully.

    static constexpr uint32_t kWorkGroupSize = 64; ///< Threads per workgroup.
};

} // namespace koilo

#endif // KL_HAVE_OPENGL_BACKEND
