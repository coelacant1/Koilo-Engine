/**
 * @file PixelGroup.h
 * @brief Declares the PixelGroup template class for managing a collection of pixels.
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
#include "../../../registry/reflect_macros.hpp"

#include "ipixelgroup.hpp" // Include for the base pixel group interface.
#include "../../../core/geometry/2d/overlap.hpp"

/**
 * @class PixelGroup
 * @brief Manages a collection of pixels with positions, colors, and spatial relationships.
 *
 * The PixelGroup class provides methods for retrieving and manipulating pixel data,
 * including spatial relationships and color properties. Supports both rectangular
 * and arbitrary pixel arrangements.
 *
 */
class PixelGroup : public IPixelGroup {
private:
    static constexpr uint16_t kInvalidIndex = 65535;

    const Vector2D* pixelPositions = nullptr; ///< Array of pixel positions.
    Direction direction = ZEROTOMAX; ///< Direction of pixel traversal.
    Rectangle2D bounds; ///< Bounding box for the pixel group.
    std::vector<RGBColor> pixelColors; ///< Array of pixel colors.
    std::vector<RGBColor> pixelBuffer; ///< Array of color buffers for temporary use.
    std::vector<uint16_t> up; ///< Indices of pixels above each pixel.
    std::vector<uint16_t> down; ///< Indices of pixels below each pixel.
    std::vector<uint16_t> left; ///< Indices of pixels to the left of each pixel.
    std::vector<uint16_t> right; ///< Indices of pixels to the right of each pixel.

    bool isRectangular = false; ///< Indicates if the group forms a rectangular grid.
    uint16_t pixelCount = 0; ///< Total number of pixels in the group.
    uint16_t rowCount = 0; ///< Number of rows in the grid.
    uint16_t colCount = 0; ///< Number of columns in the grid.
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
    PixelGroup(uint16_t pixelCount, Vector2D size, Vector2D position, uint16_t rowCount);

    /**
     * @brief Constructs a PixelGroup from arbitrary pixel locations.
     *
     * @param pixelLocations Array of pixel locations.
     * @param direction Direction of pixel traversal (default: ZEROTOMAX).
     */
    PixelGroup(const Vector2D* pixelLocations, uint16_t pixelCount, Direction direction = ZEROTOMAX);

    /**
     * @brief Destroys the PixelGroup object.
     */
    ~PixelGroup() override;

    Vector2D GetCenterCoordinate() override;
    Vector2D GetSize() override;
    Vector2D GetCoordinate(uint16_t count) override;
    int GetPixelIndex(Vector2D location) override;
    RGBColor* GetColor(uint16_t count) override;
    RGBColor* GetColors() override;
    RGBColor* GetColorBuffer() override;
    uint16_t GetPixelCount() override;
    bool Overlaps(Rectangle2D* box) override;
    bool ContainsVector2D(Vector2D v) override;
    bool GetUpIndex(uint16_t count, uint16_t* upIndex) override;
    bool GetDownIndex(uint16_t count, uint16_t* downIndex) override;
    bool GetLeftIndex(uint16_t count, uint16_t* leftIndex) override;
    bool GetRightIndex(uint16_t count, uint16_t* rightIndex) override;
    bool GetAlternateXIndex(uint16_t count, uint16_t* index) override;
    bool GetAlternateYIndex(uint16_t count, uint16_t* index) override;
    bool GetOffsetXIndex(uint16_t count, uint16_t* index, int x1) override;
    bool GetOffsetYIndex(uint16_t count, uint16_t* index, int y1) override;
    bool GetOffsetXYIndex(uint16_t count, uint16_t* index, int x1, int y1) override;
    bool GetRadialIndex(uint16_t count, uint16_t* index, int pixels, float angle) override;
    void GridSort() override;

    PTX_BEGIN_FIELDS(PixelGroup)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(PixelGroup)
        PTX_METHOD_AUTO(PixelGroup, GetCenterCoordinate, "Get center coordinate"),
        PTX_METHOD_AUTO(PixelGroup, GetSize, "Get size"),
        PTX_METHOD_AUTO(PixelGroup, GetCoordinate, "Get coordinate"),
        PTX_METHOD_AUTO(PixelGroup, GetPixelIndex, "Get pixel index"),
        PTX_METHOD_AUTO(PixelGroup, GetColor, "Get color"),
        PTX_METHOD_AUTO(PixelGroup, GetColors, "Get colors"),
        PTX_METHOD_AUTO(PixelGroup, GetColorBuffer, "Get color buffer"),
        PTX_METHOD_AUTO(PixelGroup, GetPixelCount, "Get pixel count"),
        PTX_METHOD_AUTO(PixelGroup, Overlaps, "Overlaps"),
        PTX_METHOD_AUTO(PixelGroup, ContainsVector2D, "Contains vector2 d"),
        PTX_METHOD_AUTO(PixelGroup, GetUpIndex, "Get up index"),
        PTX_METHOD_AUTO(PixelGroup, GetDownIndex, "Get down index"),
        PTX_METHOD_AUTO(PixelGroup, GetLeftIndex, "Get left index"),
        PTX_METHOD_AUTO(PixelGroup, GetRightIndex, "Get right index"),
        PTX_METHOD_AUTO(PixelGroup, GetAlternateXIndex, "Get alternate xindex"),
        PTX_METHOD_AUTO(PixelGroup, GetAlternateYIndex, "Get alternate yindex"),
        PTX_METHOD_AUTO(PixelGroup, GetOffsetXIndex, "Get offset xindex"),
        PTX_METHOD_AUTO(PixelGroup, GetOffsetYIndex, "Get offset yindex"),
        PTX_METHOD_AUTO(PixelGroup, GetOffsetXYIndex, "Get offset xyindex"),
        PTX_METHOD_AUTO(PixelGroup, GetRadialIndex, "Get radial index"),
        PTX_METHOD_AUTO(PixelGroup, GridSort, "Grid sort")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(PixelGroup)
        PTX_CTOR(PixelGroup, uint16_t, Vector2D, Vector2D, uint16_t),
        PTX_CTOR(PixelGroup, const Vector2D *, uint16_t, Direction)
    PTX_END_DESCRIBE(PixelGroup)

};
