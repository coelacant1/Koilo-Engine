// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/core/pixelgroup.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <koilo/core/math/mathematics.hpp>


namespace koilo {

koilo::PixelGroup::PixelGroup(uint32_t pixelCount, Vector2D size, Vector2D position, uint32_t rowCount)
    : bounds(position, size, 0.0f),
      pixelColors(pixelCount),
      pixelBuffer(pixelCount),
      up(pixelCount, kInvalidIndex),
      down(pixelCount, kInvalidIndex),
      left(pixelCount, kInvalidIndex),
      right(pixelCount, kInvalidIndex) {
    this->pixelCount = pixelCount;
    this->size = size;
    this->position = position;
    this->rowCount = rowCount;
    this->colCount = (rowCount > 0) ? static_cast<uint32_t>(pixelCount / rowCount) : 0;
    this->direction = IPixelGroup::Direction::ZEROTOMAX;
    this->isRectangular = true;

    bounds.UpdateBounds(position);
    bounds.UpdateBounds(position + size);

    GridSort();
}

koilo::PixelGroup::PixelGroup(const Vector2D* pixelLocations, uint32_t pixelCount, Direction direction)
    : direction(direction),
      bounds(position, size, 0.0f),
      pixelColors(pixelCount),
      pixelBuffer(pixelCount),
      up(pixelCount, kInvalidIndex),
      down(pixelCount, kInvalidIndex),
      left(pixelCount, kInvalidIndex),
      right(pixelCount, kInvalidIndex) {
    this->pixelCount = pixelCount;
    this->pixelPositions = pixelLocations;
    this->isRectangular = false;

    if (pixelLocations) {
        for (uint32_t i = 0; i < pixelCount; ++i) {
            bounds.UpdateBounds(pixelLocations[i]);
        }
    }

    GridSort();
}

koilo::PixelGroup::~PixelGroup() = default;

Vector2D koilo::PixelGroup::GetCenterCoordinate() {
    return (bounds.GetMaximum() + bounds.GetMinimum()) / 2.0f;
}

Vector2D koilo::PixelGroup::GetSize() {
    return bounds.GetMaximum() - bounds.GetMinimum();
}

Vector2D koilo::PixelGroup::GetCoordinate(uint32_t count) {
    if (pixelCount == 0) {
        return {};
    }
    if (count >= pixelCount) {
        // Match historical behaviour: clamp to the last valid index.
        count = pixelCount - 1;
    }
    EnsureCoordinatesCache();
    return coordinatesCache[count];
}

const Vector2D* koilo::PixelGroup::GetCoordinatesArray() {
    if (pixelCount == 0) return nullptr;
    EnsureCoordinatesCache();
    return coordinatesCache.data();
}

void koilo::PixelGroup::EnsureCoordinatesCache() const {
    if (coordinatesCacheValid) return;
    coordinatesCache.resize(pixelCount);
    if (pixelCount == 0) {
        coordinatesCacheValid = true;
        return;
    }

    if (isRectangular) {
        if (rowCount == 0 || colCount == 0) {
            for (auto& v : coordinatesCache) v = {};
            coordinatesCacheValid = true;
            return;
        }
        // rowCount = pixels per row (X span), colCount = number of rows (Y span).
        // Hoist the Map() linear interpolation to (offset, step) form so the
        // per-pixel cost is two multiply-adds.
        const float invRow = 1.0f / static_cast<float>(rowCount);
        const float invCol = 1.0f / static_cast<float>(colCount);
        for (uint32_t i = 0; i < pixelCount; ++i) {
            const uint32_t col = i % rowCount;
            const uint32_t row = i / rowCount;
            coordinatesCache[i].X = position.X + size.X * (static_cast<float>(col) * invRow);
            coordinatesCache[i].Y = position.Y + size.Y * (static_cast<float>(row) * invCol);
        }
    } else if (pixelPositions) {
        if (direction == ZEROTOMAX) {
            for (uint32_t i = 0; i < pixelCount; ++i) {
                coordinatesCache[i] = pixelPositions[i];
            }
        } else {
            for (uint32_t i = 0; i < pixelCount; ++i) {
                coordinatesCache[i] = pixelPositions[pixelCount - i - 1];
            }
        }
    } else {
        for (auto& v : coordinatesCache) v = {};
    }
    coordinatesCacheValid = true;
}

int koilo::PixelGroup::GetPixelIndex(Vector2D location) {
    if (!isRectangular || rowCount == 0 || colCount == 0) {
        return -1;
    }

    float row = Mathematics::Map(location.X, position.X, position.X + size.X, 0.0f, static_cast<float>(rowCount));
    float col = Mathematics::Map(location.Y, position.Y, position.Y + size.Y, 0.0f, static_cast<float>(colCount));

    uint32_t count = static_cast<uint32_t>(row + col * rowCount);

    if (count < pixelCount && count > 0 && row > 0 && row < rowCount && col > 0 && col < colCount) {
        return count;
    }

    return -1;
}

Color888* koilo::PixelGroup::GetColor(uint32_t count) {
    if (count >= pixelColors.size()) {
        return nullptr;
    }

    return &pixelColors[count];
}

Color888* koilo::PixelGroup::GetColors() {
    return pixelColors.empty() ? nullptr : pixelColors.data();
}

Color888* koilo::PixelGroup::GetColorBuffer() {
    return pixelBuffer.empty() ? nullptr : pixelBuffer.data();
}

uint32_t koilo::PixelGroup::GetPixelCount() {
    return pixelCount;
}

bool koilo::PixelGroup::Overlaps(Rectangle2D* box) {
    if (!box) {
        return false;
    }

    return bounds.Overlaps(*box);
}

bool koilo::PixelGroup::ContainsVector2D(Vector2D v) {
    return v.CheckBounds(bounds.GetMinimum(), bounds.GetMaximum());
}

bool koilo::PixelGroup::GetUpIndex(uint32_t count, uint32_t* upIndex) {
    if (!upIndex || count >= up.size()) {
        return false;
    }

    *upIndex = up[count];
    return up[count] < kInvalidIndex;
}

bool koilo::PixelGroup::GetDownIndex(uint32_t count, uint32_t* downIndex) {
    if (!downIndex || count >= down.size()) {
        return false;
    }

    *downIndex = down[count];
    return down[count] < kInvalidIndex;
}

bool koilo::PixelGroup::GetLeftIndex(uint32_t count, uint32_t* leftIndex) {
    if (!leftIndex || count >= left.size()) {
        return false;
    }

    *leftIndex = left[count];
    return left[count] < kInvalidIndex;
}

bool koilo::PixelGroup::GetRightIndex(uint32_t count, uint32_t* rightIndex) {
    if (!rightIndex || count >= right.size()) {
        return false;
    }

    *rightIndex = right[count];
    return right[count] < kInvalidIndex;
}

bool koilo::PixelGroup::GetAlternateXIndex(uint32_t count, uint32_t* index) {
    if (!index) {
        return false;
    }

    uint32_t tempIndex = count;
    bool isEven = (count % 2) != 0;
    bool valid = true;

    const uint32_t iterations = count / 2;
    for (uint32_t i = 0; i < iterations; ++i) {
        if (isEven) {
            valid = GetRightIndex(tempIndex, &tempIndex);
        } else {
            valid = GetLeftIndex(tempIndex, &tempIndex);
        }

        if (!valid) {
            break;
        }
    }

    *index = tempIndex;
    return valid;
}

bool koilo::PixelGroup::GetAlternateYIndex(uint32_t count, uint32_t* index) {
    if (!index) {
        return false;
    }

    uint32_t tempIndex = count;
    bool isEven = (count % 2) != 0;
    bool valid = true;

    const uint32_t iterations = count / 2;
    for (uint32_t i = 0; i < iterations; ++i) {
        if (isEven) {
            valid = GetUpIndex(tempIndex, &tempIndex);
        } else {
            valid = GetDownIndex(tempIndex, &tempIndex);
        }

        if (!valid) {
            break;
        }
    }

    *index = tempIndex;
    return valid;
}

bool koilo::PixelGroup::GetOffsetXIndex(uint32_t count, uint32_t* index, int x1) {
    if (!index) {
        return false;
    }

    uint32_t tempIndex = count;
    bool valid = true;

    if (x1 != 0) {
        const int steps = std::abs(x1);
        for (int i = 0; i < steps; ++i) {
            if (x1 > 0) {
                valid = GetRightIndex(tempIndex, &tempIndex);
            } else {
                valid = GetLeftIndex(tempIndex, &tempIndex);
            }

            if (!valid) {
                break;
            }
        }
    }

    *index = tempIndex;
    return valid;
}

bool koilo::PixelGroup::GetOffsetYIndex(uint32_t count, uint32_t* index, int y1) {
    if (!index) {
        return false;
    }

    uint32_t tempIndex = count;
    bool valid = true;

    if (y1 != 0) {
        const int steps = std::abs(y1);
        for (int i = 0; i < steps; ++i) {
            if (y1 > 0) {
                valid = GetUpIndex(tempIndex, &tempIndex);
            } else {
                valid = GetDownIndex(tempIndex, &tempIndex);
            }

            if (!valid) {
                break;
            }
        }
    }

    *index = tempIndex;
    return valid;
}

bool koilo::PixelGroup::GetOffsetXYIndex(uint32_t count, uint32_t* index, int x1, int y1) {
    if (!index) {
        return false;
    }

    uint32_t tempIndex = count;
    bool valid = true;

    if (x1 != 0) {
        const int stepsX = std::abs(x1);
        for (int i = 0; i < stepsX; ++i) {
            if (x1 > 0) {
                valid = GetRightIndex(tempIndex, &tempIndex);
            } else {
                valid = GetLeftIndex(tempIndex, &tempIndex);
            }

            if (!valid) {
                break;
            }
        }
    }

    if (valid && y1 != 0) {
        const int stepsY = std::abs(y1);
        for (int i = 0; i < stepsY; ++i) {
            if (y1 > 0) {
                valid = GetUpIndex(tempIndex, &tempIndex);
            } else {
                valid = GetDownIndex(tempIndex, &tempIndex);
            }

            if (!valid) {
                break;
            }
        }
    }

    *index = tempIndex;
    return valid;
}

bool koilo::PixelGroup::GetRadialIndex(uint32_t count, uint32_t* index, int pixels, float angle) {
    if (!index) {
        return false;
    }

    int x1 = static_cast<int>(static_cast<float>(pixels) * std::cos(angle * Mathematics::MPID180));
    int y1 = static_cast<int>(static_cast<float>(pixels) * std::sin(angle * Mathematics::MPID180));

    uint32_t tempIndex = count;
    bool valid = true;

    int previousX = 0;
    int previousY = 0;

    int x = 0;
    int y = 0;

    for (int i = 0; i < pixels && valid; ++i) {
        x = Mathematics::Map(i, 0, pixels, 0, x1);
        y = Mathematics::Map(i, 0, pixels, 0, y1);

        const int deltaX = std::abs(x - previousX);
        for (int k = 0; k < deltaX; ++k) {
            if (x > previousX) {
                valid = GetRightIndex(tempIndex, &tempIndex);
            } else if (x < previousX) {
                valid = GetLeftIndex(tempIndex, &tempIndex);
            }

            if (!valid) {
                break;
            }
        }

        if (!valid) {
            break;
        }

        const int deltaY = std::abs(y - previousY);
        for (int k = 0; k < deltaY; ++k) {
            if (y > previousY) {
                valid = GetUpIndex(tempIndex, &tempIndex);
            } else if (y < previousY) {
                valid = GetDownIndex(tempIndex, &tempIndex);
            }

            if (!valid) {
                break;
            }
        }

        previousX = x;
        previousY = y;
    }

    *index = tempIndex;
    return valid;
}

void koilo::PixelGroup::GridSort() {
    if (pixelCount == 0) {
        return;
    }

    if (!isRectangular) {
        if (!pixelPositions) {
            return;
        }

        for (uint32_t i = 0; i < pixelCount; ++i) {
            const Vector2D currentPos = direction == ZEROTOMAX ? pixelPositions[i] : pixelPositions[pixelCount - i - 1];

            float minUp = Mathematics::FLTMAX;
            float minDown = Mathematics::FLTMAX;
            float minLeft = Mathematics::FLTMAX;
            float minRight = Mathematics::FLTMAX;

            int minUpIndex = -1;
            int minDownIndex = -1;
            int minLeftIndex = -1;
            int minRightIndex = -1;

            for (uint32_t j = 0; j < pixelCount; ++j) {
                if (i == j) {
                    continue;
                }

                const Vector2D neighborPos = direction == ZEROTOMAX ? pixelPositions[j] : pixelPositions[pixelCount - j - 1];
                float dist = currentPos.CalculateEuclideanDistance(neighborPos);

                if (Mathematics::IsClose(currentPos.X, neighborPos.X, 1.0f)) {
                    if (currentPos.Y < neighborPos.Y && dist < minUp) {
                        minUp = dist;
                        minUpIndex = j;
                    } else if (currentPos.Y > neighborPos.Y && dist < minDown) {
                        minDown = dist;
                        minDownIndex = j;
                    }
                }

                if (Mathematics::IsClose(currentPos.Y, neighborPos.Y, 1.0f)) {
                    if (currentPos.X > neighborPos.X && dist < minLeft) {
                        minLeft = dist;
                        minLeftIndex = j;
                    } else if (currentPos.X < neighborPos.X && dist < minRight) {
                        minRight = dist;
                        minRightIndex = j;
                    }
                }
            }

            if (minUpIndex != -1) {
                up[i] = static_cast<uint32_t>(minUpIndex);
            }
            if (minDownIndex != -1) {
                down[i] = static_cast<uint32_t>(minDownIndex);
            }
            if (minLeftIndex != -1) {
                left[i] = static_cast<uint32_t>(minLeftIndex);
            }
            if (minRightIndex != -1) {
                right[i] = static_cast<uint32_t>(minRightIndex);
            }
        }

        return;
    }

    for (uint32_t i = 0; i < pixelCount; ++i) {
        if (i + rowCount < pixelCount - 1) {
            up[i] = static_cast<uint32_t>(i + rowCount);
        }
        if (i >= rowCount + 1) {
            down[i] = static_cast<uint32_t>(i - rowCount);
        }
        if (!(i % rowCount == 0) && i > 1) {
            left[i] = static_cast<uint32_t>(i - 1);
        }
        if (!(i % rowCount + 1 == 0) && i < pixelCount - 1) {
            right[i] = static_cast<uint32_t>(i + 1);
        }
    }
}

// Lua-friendly helper methods (return by value instead of pointer)
Color888 koilo::PixelGroup::GetColorAt(uint32_t index) const {
    if (index >= pixelColors.size()) {
        return Color888(0, 0, 0);  // Return black for out-of-bounds
    }
    return pixelColors[index];
}

void koilo::PixelGroup::SetColorAt(uint32_t index, const Color888& color) {
    if (index < pixelColors.size()) {
        pixelColors[index] = color;
    }
}

void koilo::PixelGroup::SetColorRGB(uint32_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < pixelColors.size()) {
        pixelColors[index].r = r;
        pixelColors[index].g = g;
        pixelColors[index].b = b;
    }
}

void koilo::PixelGroup::FillColor(const Color888& color) {
    std::fill(pixelColors.begin(), pixelColors.end(), color);
}

void koilo::PixelGroup::FillColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    Color888 color(r, g, b);
    std::fill(pixelColors.begin(), pixelColors.end(), color);
}

void koilo::PixelGroup::ClearPixels() {
    std::fill(pixelColors.begin(), pixelColors.end(), Color888(0, 0, 0));
}

} // namespace koilo
