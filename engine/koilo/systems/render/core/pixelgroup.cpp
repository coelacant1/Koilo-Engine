#include <ptx/systems/render/core/pixelgroup.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <ptx/core/math/mathematics.hpp>

PixelGroup::PixelGroup(uint16_t pixelCount, Vector2D size, Vector2D position, uint16_t rowCount)
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
    this->colCount = (rowCount > 0) ? static_cast<uint16_t>(pixelCount / rowCount) : 0;
    this->direction = IPixelGroup::Direction::ZEROTOMAX;
    this->isRectangular = true;

    bounds.UpdateBounds(position);
    bounds.UpdateBounds(position + size);

    GridSort();
}

PixelGroup::PixelGroup(const Vector2D* pixelLocations, uint16_t pixelCount, Direction direction)
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
        for (uint16_t i = 0; i < pixelCount; ++i) {
            bounds.UpdateBounds(pixelLocations[i]);
        }
    }

    GridSort();
}

PixelGroup::~PixelGroup() = default;

Vector2D PixelGroup::GetCenterCoordinate() {
    return (bounds.GetMaximum() + bounds.GetMinimum()) / 2.0f;
}

Vector2D PixelGroup::GetSize() {
    return bounds.GetMaximum() - bounds.GetMinimum();
}

Vector2D PixelGroup::GetCoordinate(uint16_t count) {
    if (pixelCount == 0) {
        return {};
    }

    count = Mathematics::Constrain<int>(count, 0, pixelCount);

    if (isRectangular) {
        if (rowCount == 0 || colCount == 0) {
            return {};
        }

        float row = static_cast<float>(count % rowCount);
        float col = static_cast<float>((count - static_cast<uint16_t>(row)) / rowCount);

        tempLocation.X = Mathematics::Map(row, 0.0f, static_cast<float>(rowCount), position.X, position.X + size.X);
        tempLocation.Y = Mathematics::Map(col, 0.0f, static_cast<float>(colCount), position.Y, position.Y + size.Y);

        return tempLocation;
    }

    if (!pixelPositions) {
        return {};
    }

    if (direction == ZEROTOMAX) {
        return pixelPositions[count];
    }

    return pixelPositions[pixelCount - count - 1];
}

int PixelGroup::GetPixelIndex(Vector2D location) {
    if (!isRectangular || rowCount == 0 || colCount == 0) {
        return -1;
    }

    float row = Mathematics::Map(location.X, position.X, position.X + size.X, 0.0f, static_cast<float>(rowCount));
    float col = Mathematics::Map(location.Y, position.Y, position.Y + size.Y, 0.0f, static_cast<float>(colCount));

    uint16_t count = static_cast<uint16_t>(row + col * rowCount);

    if (count < pixelCount && count > 0 && row > 0 && row < rowCount && col > 0 && col < colCount) {
        return count;
    }

    return -1;
}

RGBColor* PixelGroup::GetColor(uint16_t count) {
    if (count >= pixelColors.size()) {
        return nullptr;
    }

    return &pixelColors[count];
}

RGBColor* PixelGroup::GetColors() {
    return pixelColors.empty() ? nullptr : pixelColors.data();
}

RGBColor* PixelGroup::GetColorBuffer() {
    return pixelBuffer.empty() ? nullptr : pixelBuffer.data();
}

uint16_t PixelGroup::GetPixelCount() {
    return pixelCount;
}

bool PixelGroup::Overlaps(Rectangle2D* box) {
    if (!box) {
        return false;
    }

    return Overlap2D::Overlaps(bounds, *box);
}

bool PixelGroup::ContainsVector2D(Vector2D v) {
    return v.CheckBounds(bounds.GetMinimum(), bounds.GetMaximum());
}

bool PixelGroup::GetUpIndex(uint16_t count, uint16_t* upIndex) {
    if (!upIndex || count >= up.size()) {
        return false;
    }

    *upIndex = up[count];
    return up[count] < kInvalidIndex;
}

bool PixelGroup::GetDownIndex(uint16_t count, uint16_t* downIndex) {
    if (!downIndex || count >= down.size()) {
        return false;
    }

    *downIndex = down[count];
    return down[count] < kInvalidIndex;
}

bool PixelGroup::GetLeftIndex(uint16_t count, uint16_t* leftIndex) {
    if (!leftIndex || count >= left.size()) {
        return false;
    }

    *leftIndex = left[count];
    return left[count] < kInvalidIndex;
}

bool PixelGroup::GetRightIndex(uint16_t count, uint16_t* rightIndex) {
    if (!rightIndex || count >= right.size()) {
        return false;
    }

    *rightIndex = right[count];
    return right[count] < kInvalidIndex;
}

bool PixelGroup::GetAlternateXIndex(uint16_t count, uint16_t* index) {
    if (!index) {
        return false;
    }

    uint16_t tempIndex = count;
    bool isEven = (count % 2) != 0;
    bool valid = true;

    const uint16_t iterations = count / 2;
    for (uint16_t i = 0; i < iterations; ++i) {
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

bool PixelGroup::GetAlternateYIndex(uint16_t count, uint16_t* index) {
    if (!index) {
        return false;
    }

    uint16_t tempIndex = count;
    bool isEven = (count % 2) != 0;
    bool valid = true;

    const uint16_t iterations = count / 2;
    for (uint16_t i = 0; i < iterations; ++i) {
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

bool PixelGroup::GetOffsetXIndex(uint16_t count, uint16_t* index, int x1) {
    if (!index) {
        return false;
    }

    uint16_t tempIndex = count;
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

bool PixelGroup::GetOffsetYIndex(uint16_t count, uint16_t* index, int y1) {
    if (!index) {
        return false;
    }

    uint16_t tempIndex = count;
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

bool PixelGroup::GetOffsetXYIndex(uint16_t count, uint16_t* index, int x1, int y1) {
    if (!index) {
        return false;
    }

    uint16_t tempIndex = count;
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

bool PixelGroup::GetRadialIndex(uint16_t count, uint16_t* index, int pixels, float angle) {
    if (!index) {
        return false;
    }

    int x1 = static_cast<int>(static_cast<float>(pixels) * std::cos(angle * Mathematics::MPID180));
    int y1 = static_cast<int>(static_cast<float>(pixels) * std::sin(angle * Mathematics::MPID180));

    uint16_t tempIndex = count;
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

void PixelGroup::GridSort() {
    if (pixelCount == 0) {
        return;
    }

    if (!isRectangular) {
        if (!pixelPositions) {
            return;
        }

        for (uint16_t i = 0; i < pixelCount; ++i) {
            const Vector2D currentPos = direction == ZEROTOMAX ? pixelPositions[i] : pixelPositions[pixelCount - i - 1];

            float minUp = Mathematics::FLTMAX;
            float minDown = Mathematics::FLTMAX;
            float minLeft = Mathematics::FLTMAX;
            float minRight = Mathematics::FLTMAX;

            int minUpIndex = -1;
            int minDownIndex = -1;
            int minLeftIndex = -1;
            int minRightIndex = -1;

            for (uint16_t j = 0; j < pixelCount; ++j) {
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
                up[i] = static_cast<uint16_t>(minUpIndex);
            }
            if (minDownIndex != -1) {
                down[i] = static_cast<uint16_t>(minDownIndex);
            }
            if (minLeftIndex != -1) {
                left[i] = static_cast<uint16_t>(minLeftIndex);
            }
            if (minRightIndex != -1) {
                right[i] = static_cast<uint16_t>(minRightIndex);
            }
        }

        return;
    }

    for (uint16_t i = 0; i < pixelCount; ++i) {
        if (i + rowCount < pixelCount - 1) {
            up[i] = static_cast<uint16_t>(i + rowCount);
        }
        if (i >= rowCount + 1) {
            down[i] = static_cast<uint16_t>(i - rowCount);
        }
        if (!(i % rowCount == 0) && i > 1) {
            left[i] = static_cast<uint16_t>(i - 1);
        }
        if (!(i % rowCount + 1 == 0) && i < pixelCount - 1) {
            right[i] = static_cast<uint16_t>(i + 1);
        }
    }
}
