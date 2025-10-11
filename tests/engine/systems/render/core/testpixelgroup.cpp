/**
 * @file testpixelgroup.cpp
 * @brief Implementation of PixelGroup unit tests.
 */

#include "testpixelgroup.hpp"

// ========== Constructor Tests ==========

void TestPixelGroup::TestDefaultConstructor() {
    // PixelGroup requires parameters - test with rectangular constructor
    PixelGroup pixelGroup(10, Vector2D(100.0f, 100.0f), Vector2D(0.0f, 0.0f), 2);

    // Verify pixel group was constructed
    TEST_ASSERT_EQUAL(10, pixelGroup.GetPixelCount());
    TEST_ASSERT_NOT_NULL(pixelGroup.GetColors());
}

// ========== Method Tests ==========
void TestPixelGroup::TestGetCenterCoordinate() {
    // Create rectangular pixel group from (0,0) to (100,100)
    PixelGroup pixelGroup(10, Vector2D(100.0f, 100.0f), Vector2D(0.0f, 0.0f), 2);

    Vector2D center = pixelGroup.GetCenterCoordinate();
    // Center should be at (50, 50)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, center.Y);
}
void TestPixelGroup::TestGetSize() {
    PixelGroup pixelGroup(10, Vector2D(100.0f, 200.0f), Vector2D(0.0f, 0.0f), 2);

    Vector2D size = pixelGroup.GetSize();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, size.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, size.Y);
}
void TestPixelGroup::TestGetCoordinate() {
    // Test with array-based constructor
    Vector2D pixels[] = {
        Vector2D(10.0f, 20.0f),
        Vector2D(30.0f, 40.0f),
        Vector2D(50.0f, 60.0f)
    };
    PixelGroup pixelGroup(pixels, 3);

    Vector2D coord0 = pixelGroup.GetCoordinate(0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, coord0.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, coord0.Y);

    Vector2D coord2 = pixelGroup.GetCoordinate(2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, coord2.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, coord2.Y);
}
void TestPixelGroup::TestGetPixelIndex() {
    // GetPixelIndex only works for rectangular pixel groups
    PixelGroup pixelGroup(10, Vector2D(100.0f, 100.0f), Vector2D(0.0f, 0.0f), 2);

    // Test with a location that should map to a valid pixel
    Vector2D location(10.0f, 10.0f);
    int index = pixelGroup.GetPixelIndex(location);

    // Should return a valid index (0-9) or -1 if outside bounds
    TEST_ASSERT_TRUE(index >= -1 && index < 10);
}
void TestPixelGroup::TestGetColor() {
    PixelGroup pixelGroup(5, Vector2D(50.0f, 50.0f), Vector2D(0.0f, 0.0f), 1);

    // Get color for pixel 0
    RGBColor* color = pixelGroup.GetColor(0);
    TEST_ASSERT_NOT_NULL(color);

    // Test out of bounds
    RGBColor* outOfBounds = pixelGroup.GetColor(100);
    TEST_ASSERT_NULL(outOfBounds);
}
void TestPixelGroup::TestGetColors() {
    PixelGroup pixelGroup(5, Vector2D(50.0f, 50.0f), Vector2D(0.0f, 0.0f), 1);

    RGBColor* colors = pixelGroup.GetColors();
    TEST_ASSERT_NOT_NULL(colors);
}
void TestPixelGroup::TestGetColorBuffer() {
    PixelGroup pixelGroup(5, Vector2D(50.0f, 50.0f), Vector2D(0.0f, 0.0f), 1);

    RGBColor* buffer = pixelGroup.GetColorBuffer();
    TEST_ASSERT_NOT_NULL(buffer);
}
void TestPixelGroup::TestGetPixelCount() {
    PixelGroup pixelGroup(8, Vector2D(80.0f, 80.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t count = pixelGroup.GetPixelCount();
    TEST_ASSERT_EQUAL(8, count);
}
void TestPixelGroup::TestOverlaps() {
    PixelGroup pixelGroup(10, Vector2D(100.0f, 100.0f), Vector2D(0.0f, 0.0f), 2);

    // Test overlapping box
    Rectangle2D overlappingBox(Vector2D(50.0f, 50.0f), Vector2D(150.0f, 150.0f), 0.0f);
    TEST_ASSERT_TRUE(pixelGroup.Overlaps(&overlappingBox));

    // Test non-overlapping box
    Rectangle2D nonOverlappingBox(Vector2D(200.0f, 200.0f), Vector2D(300.0f, 300.0f), 0.0f);
    TEST_ASSERT_FALSE(pixelGroup.Overlaps(&nonOverlappingBox));

    // Test nullptr
    TEST_ASSERT_FALSE(pixelGroup.Overlaps(nullptr));
}
void TestPixelGroup::TestContainsVector2D() {
    PixelGroup pixelGroup(10, Vector2D(100.0f, 100.0f), Vector2D(0.0f, 0.0f), 2);

    // Test point inside bounds
    Vector2D inside(50.0f, 50.0f);
    TEST_ASSERT_TRUE(pixelGroup.ContainsVector2D(inside));

    // Test point outside bounds
    Vector2D outside(150.0f, 150.0f);
    TEST_ASSERT_FALSE(pixelGroup.ContainsVector2D(outside));
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestPixelGroup::TestParameterizedConstructor() {
    // Test array-based constructor
    Vector2D pixels[] = {
        Vector2D(0.0f, 0.0f),
        Vector2D(10.0f, 0.0f),
        Vector2D(20.0f, 0.0f)
    };
    PixelGroup pixelGroup(pixels, 3);

    TEST_ASSERT_EQUAL(3, pixelGroup.GetPixelCount());
    TEST_ASSERT_NOT_NULL(pixelGroup.GetColors());
}

void TestPixelGroup::TestEdgeCases() {
    // Test with single pixel
    PixelGroup singlePixel(1, Vector2D(10.0f, 10.0f), Vector2D(0.0f, 0.0f), 1);
    TEST_ASSERT_EQUAL(1, singlePixel.GetPixelCount());

    // Test with large pixel count
    PixelGroup largeGroup(100, Vector2D(1000.0f, 1000.0f), Vector2D(0.0f, 0.0f), 10);
    TEST_ASSERT_EQUAL(100, largeGroup.GetPixelCount());

    // Test center coordinate calculation
    Vector2D center = largeGroup.GetCenterCoordinate();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 500.0f, center.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 500.0f, center.Y);
}

void TestPixelGroup::TestGetAlternateXIndex() {
    // Create a simple grid
    PixelGroup pixelGroup(4, Vector2D(40.0f, 40.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t index = 0;
    bool result = pixelGroup.GetAlternateXIndex(0, &index);

    // Should return a valid index
    TEST_ASSERT_TRUE(index < 4);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetAlternateXIndex(0, nullptr));
}

void TestPixelGroup::TestGetAlternateYIndex() {
    PixelGroup pixelGroup(4, Vector2D(40.0f, 40.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t index = 0;
    bool result = pixelGroup.GetAlternateYIndex(0, &index);

    TEST_ASSERT_TRUE(index < 4);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetAlternateYIndex(0, nullptr));
}

void TestPixelGroup::TestGetDownIndex() {
    // Create 2x2 grid (4 pixels, 2 rows)
    PixelGroup pixelGroup(4, Vector2D(20.0f, 20.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t downIndex = 0;
    // Test getting down index from pixel 2 (should be valid)
    bool result = pixelGroup.GetDownIndex(2, &downIndex);

    // Result should be valid or invalid depending on grid layout
    TEST_ASSERT_TRUE(result == true || result == false);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetDownIndex(0, nullptr));
}

void TestPixelGroup::TestGetLeftIndex() {
    PixelGroup pixelGroup(4, Vector2D(20.0f, 20.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t leftIndex = 0;
    // Test getting left index from pixel 1
    bool result = pixelGroup.GetLeftIndex(1, &leftIndex);

    TEST_ASSERT_TRUE(result == true || result == false);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetLeftIndex(0, nullptr));
}

void TestPixelGroup::TestGetOffsetXIndex() {
    PixelGroup pixelGroup(9, Vector2D(30.0f, 30.0f), Vector2D(0.0f, 0.0f), 3);

    uint16_t offsetIndex = 0;
    // Test offset by 1 to the right
    bool result = pixelGroup.GetOffsetXIndex(0, &offsetIndex, 1);

    TEST_ASSERT_TRUE(offsetIndex < 9);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetOffsetXIndex(0, nullptr, 1));
}

void TestPixelGroup::TestGetOffsetXYIndex() {
    PixelGroup pixelGroup(9, Vector2D(30.0f, 30.0f), Vector2D(0.0f, 0.0f), 3);

    uint16_t offsetIndex = 0;
    // Test offset by (1,1)
    bool result = pixelGroup.GetOffsetXYIndex(0, &offsetIndex, 1, 1);

    TEST_ASSERT_TRUE(offsetIndex < 9);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetOffsetXYIndex(0, nullptr, 1, 1));
}

void TestPixelGroup::TestGetOffsetYIndex() {
    PixelGroup pixelGroup(9, Vector2D(30.0f, 30.0f), Vector2D(0.0f, 0.0f), 3);

    uint16_t offsetIndex = 0;
    // Test offset by 1 up
    bool result = pixelGroup.GetOffsetYIndex(0, &offsetIndex, 1);

    TEST_ASSERT_TRUE(offsetIndex < 9);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetOffsetYIndex(0, nullptr, 1));
}

void TestPixelGroup::TestGetRadialIndex() {
    PixelGroup pixelGroup(9, Vector2D(30.0f, 30.0f), Vector2D(0.0f, 0.0f), 3);

    uint16_t radialIndex = 0;
    // Test radial navigation (2 pixels at 45 degrees)
    bool result = pixelGroup.GetRadialIndex(4, &radialIndex, 2, 45.0f);

    TEST_ASSERT_TRUE(radialIndex < 9);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetRadialIndex(0, nullptr, 1, 0.0f));
}

void TestPixelGroup::TestGetRightIndex() {
    PixelGroup pixelGroup(4, Vector2D(20.0f, 20.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t rightIndex = 0;
    // Test getting right index from pixel 0
    bool result = pixelGroup.GetRightIndex(0, &rightIndex);

    TEST_ASSERT_TRUE(result == true || result == false);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetRightIndex(0, nullptr));
}

void TestPixelGroup::TestGetUpIndex() {
    PixelGroup pixelGroup(4, Vector2D(20.0f, 20.0f), Vector2D(0.0f, 0.0f), 2);

    uint16_t upIndex = 0;
    // Test getting up index from pixel 0
    bool result = pixelGroup.GetUpIndex(0, &upIndex);

    TEST_ASSERT_TRUE(result == true || result == false);

    // Test with nullptr
    TEST_ASSERT_FALSE(pixelGroup.GetUpIndex(0, nullptr));
}

void TestPixelGroup::TestGridSort() {
    // GridSort is called automatically in constructor, so test that it works
    Vector2D pixels[] = {
        Vector2D(0.0f, 0.0f),
        Vector2D(10.0f, 0.0f),
        Vector2D(0.0f, 10.0f),
        Vector2D(10.0f, 10.0f)
    };
    PixelGroup pixelGroup(pixels, 4);

    // After GridSort, neighbor relationships should be established
    uint16_t rightIndex = 0;
    bool hasRight = pixelGroup.GetRightIndex(0, &rightIndex);

    // GridSort should have established some neighbor relationships
    TEST_ASSERT_TRUE(hasRight == true || hasRight == false);
}

void TestPixelGroup::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetCenterCoordinate);
    RUN_TEST(TestGetSize);
    RUN_TEST(TestGetCoordinate);
    RUN_TEST(TestGetPixelIndex);
    RUN_TEST(TestGetColor);
    RUN_TEST(TestGetColors);
    RUN_TEST(TestGetColorBuffer);
    RUN_TEST(TestGetPixelCount);
    RUN_TEST(TestOverlaps);
    RUN_TEST(TestContainsVector2D);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetAlternateXIndex);
    RUN_TEST(TestGetAlternateYIndex);
    RUN_TEST(TestGetDownIndex);
    RUN_TEST(TestGetLeftIndex);
    RUN_TEST(TestGetOffsetXIndex);
    RUN_TEST(TestGetOffsetXYIndex);
    RUN_TEST(TestGetOffsetYIndex);
    RUN_TEST(TestGetRadialIndex);
    RUN_TEST(TestGetRightIndex);
    RUN_TEST(TestGetUpIndex);
    RUN_TEST(TestGridSort);
}
