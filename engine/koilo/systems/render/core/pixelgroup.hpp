// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file PixelGroup.h
 * @brief Declares the PixelGroup class for managing a collection of pixels.
 *
 * This file defines the PixelGroup class, which implements the IPixelGroup interface
 * to manage a fixed number of pixels with spatial and color properties.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

#include "ipixelgroup.hpp" // Include for the base pixel group interface.


namespace koilo {

/**
 * @class PixelGroup
 * @brief Manages a collection of pixels with positions, colors, and spatial relationships.
 *
 * The PixelGroup class provides methods for retrieving and manipulating pixel data,
 * including spatial relationships and color properties. Supports both rectangular
 * and arbitrary pixel arrangements.
 *
 */
class PixelGroup final : public IPixelGroup {
private:
    static constexpr uint32_t kInvalidIndex = 65535;

    const Vector2D* pixelPositions = nullptr; ///< Array of pixel positions.
    Direction direction = ZEROTOMAX; ///< Direction of pixel traversal.
    Rectangle2D bounds; ///< Bounding box for the pixel group.
    std::vector<Color888> pixelColors; ///< Array of pixel colors.
    std::vector<Color888> pixelBuffer; ///< Array of color buffers for temporary use.
    std::vector<uint32_t> up; ///< Indices of pixels above each pixel.
    std::vector<uint32_t> down; ///< Indices of pixels below each pixel.
    std::vector<uint32_t> left; ///< Indices of pixels to the left of each pixel.
    std::vector<uint32_t> right; ///< Indices of pixels to the right of each pixel.

    /// Pre-computed per-pixel coordinates. The pixel layout is invariant
    /// for the lifetime of the PixelGroup, so we materialise the entire
    /// coordinate buffer once and serve all subsequent GetCoordinate /
    /// GetCoordinatesArray calls from it. Avoids per-call modulo,
    /// division, and Mathematics::Map work in tight render loops.
    mutable std::vector<Vector2D> coordinatesCache;
    mutable bool coordinatesCacheValid = false;

    void EnsureCoordinatesCache() const;

    bool isRectangular = false; ///< Indicates if the group forms a rectangular grid.
    uint32_t pixelCount = 0; ///< Total number of pixels in the group.
    uint32_t rowCount = 0; ///< Number of rows in the grid.
    uint32_t colCount = 0; ///< Number of columns in the grid.
    Vector2D size; ///< Size of the grid.
    Vector2D position; ///< Position of the grid.
    Vector2D tempLocation; ///< Temporary location for calculations.

public:
    /**
     * @brief Constructs a rectangular PixelGroup.
     *
     * @param size Size of the rectangular grid.
     * @param position Position of the rectangular grid.
     * @param rowCount Number of rows in the grid.
     */
    PixelGroup(uint32_t pixelCount, Vector2D size, Vector2D position, uint32_t rowCount);

    /**
     * @brief Constructs a PixelGroup from arbitrary pixel locations.
     *
     * @param pixelLocations Array of pixel locations.
     * @param direction Direction of pixel traversal (default: ZEROTOMAX).
     */
    PixelGroup(const Vector2D* pixelLocations, uint32_t pixelCount, Direction direction = ZEROTOMAX);

    /**
     * @brief Destroys the PixelGroup object.
     */
    ~PixelGroup() override;

    Vector2D GetCenterCoordinate() override;
    Vector2D GetSize() override;
    Vector2D GetCoordinate(uint32_t count) override;
    const Vector2D* GetCoordinatesArray() override;
    int GetPixelIndex(Vector2D location) override;
    Color888* GetColor(uint32_t count) override;
    Color888* GetColors() override;
    Color888* GetColorBuffer() override;
    uint32_t GetPixelCount() override;
    bool Overlaps(Rectangle2D* box) override;
    bool ContainsVector2D(Vector2D v) override;
    bool GetUpIndex(uint32_t count, uint32_t* upIndex) override;
    bool GetDownIndex(uint32_t count, uint32_t* downIndex) override;
    bool GetLeftIndex(uint32_t count, uint32_t* leftIndex) override;
    bool GetRightIndex(uint32_t count, uint32_t* rightIndex) override;
    bool GetAlternateXIndex(uint32_t count, uint32_t* index) override;
    bool GetAlternateYIndex(uint32_t count, uint32_t* index) override;
    bool GetOffsetXIndex(uint32_t count, uint32_t* index, int x1) override;
    bool GetOffsetYIndex(uint32_t count, uint32_t* index, int y1) override;
    bool GetOffsetXYIndex(uint32_t count, uint32_t* index, int x1, int y1) override;
    bool GetRadialIndex(uint32_t count, uint32_t* index, int pixels, float angle) override;
    void GridSort() override;

    // Lua-friendly helper methods (return by value instead of pointer)
    Color888 GetColorAt(uint32_t index) const;
    void SetColorAt(uint32_t index, const Color888& color);
    void SetColorRGB(uint32_t index, uint8_t r, uint8_t g, uint8_t b);
    void FillColor(const Color888& color);
    void FillColorRGB(uint8_t r, uint8_t g, uint8_t b);
    void ClearPixels();
    
    // Rectangular grid accessors
    bool IsRectangular() const { return isRectangular; }
    uint32_t GetRowCount() const { return rowCount; }
    uint32_t GetColumnCount() const { return colCount; }

    KL_BEGIN_FIELDS(PixelGroup)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(PixelGroup)
        KL_METHOD_AUTO(PixelGroup, GetCenterCoordinate, "Get center coordinate"),
        KL_METHOD_AUTO(PixelGroup, GetSize, "Get size"),
        KL_METHOD_AUTO(PixelGroup, GetCoordinate, "Get coordinate"),
        KL_METHOD_AUTO(PixelGroup, GetPixelIndex, "Get pixel index"),
        KL_METHOD_AUTO(PixelGroup, GetColor, "Get color"),
        KL_METHOD_AUTO(PixelGroup, GetColors, "Get colors"),
        KL_METHOD_AUTO(PixelGroup, GetColorBuffer, "Get color buffer"),
        KL_METHOD_AUTO(PixelGroup, GetPixelCount, "Get pixel count"),
        KL_METHOD_AUTO(PixelGroup, Overlaps, "Overlaps"),
        KL_METHOD_AUTO(PixelGroup, ContainsVector2D, "Contains vector2 d"),
        KL_METHOD_AUTO(PixelGroup, GetUpIndex, "Get up index"),
        KL_METHOD_AUTO(PixelGroup, GetDownIndex, "Get down index"),
        KL_METHOD_AUTO(PixelGroup, GetLeftIndex, "Get left index"),
        KL_METHOD_AUTO(PixelGroup, GetRightIndex, "Get right index"),
        KL_METHOD_AUTO(PixelGroup, GetAlternateXIndex, "Get alternate xindex"),
        KL_METHOD_AUTO(PixelGroup, GetAlternateYIndex, "Get alternate yindex"),
        KL_METHOD_AUTO(PixelGroup, GetOffsetXIndex, "Get offset xindex"),
        KL_METHOD_AUTO(PixelGroup, GetOffsetYIndex, "Get offset yindex"),
        KL_METHOD_AUTO(PixelGroup, GetOffsetXYIndex, "Get offset xyindex"),
        KL_METHOD_AUTO(PixelGroup, GetRadialIndex, "Get radial index"),
        KL_METHOD_AUTO(PixelGroup, GridSort, "Grid sort"),
        KL_METHOD_AUTO(PixelGroup, GetColorAt, "Get color at index"),
        KL_METHOD_AUTO(PixelGroup, SetColorAt, "Set color at index"),
        KL_METHOD_AUTO(PixelGroup, SetColorRGB, "Set pixel RGB values"),
        KL_METHOD_AUTO(PixelGroup, FillColor, "Fill all pixels with color"),
        KL_METHOD_AUTO(PixelGroup, FillColorRGB, "Fill all pixels with RGB values"),
        KL_METHOD_AUTO(PixelGroup, ClearPixels, "Clear all pixels to black")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PixelGroup)
        KL_CTOR(PixelGroup, uint32_t, Vector2D, Vector2D, uint32_t),
        KL_CTOR(PixelGroup, const Vector2D *, uint32_t, Direction)
    KL_END_DESCRIBE(PixelGroup)

};

} // namespace koilo
