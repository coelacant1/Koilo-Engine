// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file camera_layout.hpp
 * @brief Loads .klcam JSON camera layout files and normalizes to UV coordinates.
 *
 * LEDCameraLayout reads a .klcam JSON file containing LED positions in
 * physical units (millimeters), normalizes them to 0-1 UV space by dividing
 * by the max bounding box extent, and exposes a contiguous Vector2D array
 * suitable for PixelGroup construction.
 *
 * This class owns the position data and must outlive any PixelGroup or
 * VolumeCamera that references it.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace koilo {

class PixelGroup;

/**
 * @class LEDCameraLayout
 * @brief Loads and normalizes LED camera layout data from .klcam JSON files.
 *
 * The .klcam format stores LED positions in physical units (mm). On load,
 * positions are normalized to 0-1 UV space by dividing all coordinates by the
 * maximum bounding box extent (max of width, height). This preserves the
 * physical aspect ratio.
 *
 * Raw (mm) positions are also retained for diagnostics and display.
 */
class LEDCameraLayout {
public:
    LEDCameraLayout() = default;

    /**
     * @brief Load a .klcam JSON file.
     *
     * @param path File path to a .klcam JSON file.
     * @return True on success, false on parse error or file not found.
     */
    bool LoadFromFile(const std::string& path);

    /**
     * @brief Parse .klcam JSON from an in-memory string.
     *
     * @param json The JSON string contents.
     * @return True on success, false on parse error.
     */
    bool LoadFromString(const std::string& json);

    /**
     * @brief Create a PixelGroup from the normalized UV positions.
     *
     * The returned PixelGroup borrows the internal Vector2D array, so this
     * CameraLayout must remain alive for the lifetime of the PixelGroup.
     *
     * @return A heap-allocated PixelGroup, or nullptr if no data loaded.
     */
    PixelGroup* CreatePixelGroup() const;

    /// @brief Get the layout name from the file.
    const std::string& GetName() const { return name_; }

    /// @brief Get the number of LEDs/pixels.
    uint32_t GetCount() const { return static_cast<uint32_t>(normalizedPositions_.size()); }

    /// @brief Get the normalized 0-1 UV positions array.
    const Vector2D* GetNormalizedPositions() const;

    /// @brief Get the raw positions in physical units (mm).
    const Vector2D* GetRawPositions() const;

    /// @brief Get the physical bounding box size in mm.
    Vector2D GetPhysicalSize() const { return physicalSize_; }

    /// @brief Get the max extent used for normalization (mm).
    float GetNormalizationExtent() const { return normExtent_; }

    /// @brief Check if data has been loaded.
    bool IsLoaded() const { return !normalizedPositions_.empty(); }

    /// @brief Mirror all normalized U coordinates (1-u). Useful for panels
    ///        whose physical X=0 is on the right side.
    void FlipX();

    /// @brief Mirror all normalized V coordinates (1-v).
    void FlipY();

private:
    std::string name_;
    std::vector<Vector2D> rawPositions_;         ///< Positions in mm.
    std::vector<Vector2D> normalizedPositions_;  ///< Positions in 0-1 UV.
    Vector2D physicalSize_;                      ///< Bounding box in mm.
    float normExtent_ = 0.0f;                    ///< Max(width, height) in mm.

    /**
     * @brief Compute bounding box and normalize raw positions to 0-1 UV.
     */
    void Normalize();
};

} // namespace koilo
