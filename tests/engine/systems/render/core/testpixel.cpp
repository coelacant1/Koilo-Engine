/**
 * @file testpixel.cpp
 * @brief Implementation of Pixel unit tests.
 */

#include "testpixel.hpp"

// ========== Constructor Tests ==========

void TestPixel::TestDefaultConstructor() {
    Pixel pixel;

    // Verify pixel was constructed
    // Default constructor doesn't set position, so these should be false
    TEST_ASSERT_FALSE(pixel.HasUpPixel());
    TEST_ASSERT_FALSE(pixel.HasDownPixel());
    TEST_ASSERT_FALSE(pixel.HasLeftPixel());
    TEST_ASSERT_FALSE(pixel.HasRightPixel());
}

// ========== Method Tests ==========
void TestPixel::TestGetPosition() {
    Vector2D position(10.0f, 20.0f);
    Pixel pixel(&position);

    Vector2D retrieved = pixel.GetPosition();
    TEST_ASSERT_EQUAL_FLOAT(10.0f, retrieved.X);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, retrieved.Y);
}

void TestPixel::TestSetUpPixel() {
    Pixel centerPixel;
    Pixel upPixel;

    centerPixel.SetUpPixel(&upPixel);

    TEST_ASSERT_TRUE(centerPixel.HasUpPixel());
    TEST_ASSERT_EQUAL_PTR(&upPixel, centerPixel.GetUpPixel());
}

void TestPixel::TestSetDownPixel() {
    Pixel centerPixel;
    Pixel downPixel;

    centerPixel.SetDownPixel(&downPixel);

    TEST_ASSERT_TRUE(centerPixel.HasDownPixel());
    TEST_ASSERT_EQUAL_PTR(&downPixel, centerPixel.GetDownPixel());
}

void TestPixel::TestSetLeftPixel() {
    Pixel centerPixel;
    Pixel leftPixel;

    centerPixel.SetLeftPixel(&leftPixel);

    TEST_ASSERT_TRUE(centerPixel.HasLeftPixel());
    TEST_ASSERT_EQUAL_PTR(&leftPixel, centerPixel.GetLeftPixel());
}

void TestPixel::TestSetRightPixel() {
    Pixel centerPixel;
    Pixel rightPixel;

    centerPixel.SetRightPixel(&rightPixel);

    TEST_ASSERT_TRUE(centerPixel.HasRightPixel());
    TEST_ASSERT_EQUAL_PTR(&rightPixel, centerPixel.GetRightPixel());
}

void TestPixel::TestHasUpPixel() {
    Pixel pixel;

    TEST_ASSERT_FALSE(pixel.HasUpPixel());

    Pixel upPixel;
    pixel.SetUpPixel(&upPixel);

    TEST_ASSERT_TRUE(pixel.HasUpPixel());
}

void TestPixel::TestHasDownPixel() {
    Pixel pixel;

    TEST_ASSERT_FALSE(pixel.HasDownPixel());

    Pixel downPixel;
    pixel.SetDownPixel(&downPixel);

    TEST_ASSERT_TRUE(pixel.HasDownPixel());
}

void TestPixel::TestHasLeftPixel() {
    Pixel pixel;

    TEST_ASSERT_FALSE(pixel.HasLeftPixel());

    Pixel leftPixel;
    pixel.SetLeftPixel(&leftPixel);

    TEST_ASSERT_TRUE(pixel.HasLeftPixel());
}

void TestPixel::TestHasRightPixel() {
    Pixel pixel;

    TEST_ASSERT_FALSE(pixel.HasRightPixel());

    Pixel rightPixel;
    pixel.SetRightPixel(&rightPixel);

    TEST_ASSERT_TRUE(pixel.HasRightPixel());
}

void TestPixel::TestGetUpPixel() {
    Pixel centerPixel;
    Pixel upPixel;

    centerPixel.SetUpPixel(&upPixel);

    Pixel* retrieved = centerPixel.GetUpPixel();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(&upPixel, retrieved);
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestPixel::TestParameterizedConstructor() {
    // Test parameterized constructor with position
    Vector2D position(5.0f, 15.0f);
    Pixel pixel(&position);

    Vector2D retrieved = pixel.GetPosition();
    TEST_ASSERT_EQUAL_FLOAT(5.0f, retrieved.X);
    TEST_ASSERT_EQUAL_FLOAT(15.0f, retrieved.Y);
}

void TestPixel::TestEdgeCases() {
    // Test pixel grid with all neighbors
    Pixel center;
    Pixel up, down, left, right;

    center.SetUpPixel(&up);
    center.SetDownPixel(&down);
    center.SetLeftPixel(&left);
    center.SetRightPixel(&right);

    // Verify all neighbors are set
    TEST_ASSERT_TRUE(center.HasUpPixel());
    TEST_ASSERT_TRUE(center.HasDownPixel());
    TEST_ASSERT_TRUE(center.HasLeftPixel());
    TEST_ASSERT_TRUE(center.HasRightPixel());

    // Verify correct neighbors
    TEST_ASSERT_EQUAL_PTR(&up, center.GetUpPixel());
    TEST_ASSERT_EQUAL_PTR(&down, center.GetDownPixel());
    TEST_ASSERT_EQUAL_PTR(&left, center.GetLeftPixel());
    TEST_ASSERT_EQUAL_PTR(&right, center.GetRightPixel());

    // Test edge pixel with no neighbors
    Pixel edgePixel;
    TEST_ASSERT_FALSE(edgePixel.HasUpPixel());
    TEST_ASSERT_FALSE(edgePixel.HasDownPixel());
    TEST_ASSERT_FALSE(edgePixel.HasLeftPixel());
    TEST_ASSERT_FALSE(edgePixel.HasRightPixel());
    TEST_ASSERT_NULL(edgePixel.GetUpPixel());
    TEST_ASSERT_NULL(edgePixel.GetDownPixel());
    TEST_ASSERT_NULL(edgePixel.GetLeftPixel());
    TEST_ASSERT_NULL(edgePixel.GetRightPixel());
}

void TestPixel::TestGetDownPixel() {
    Pixel centerPixel;
    Pixel downPixel;

    centerPixel.SetDownPixel(&downPixel);

    Pixel* retrieved = centerPixel.GetDownPixel();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(&downPixel, retrieved);
}

void TestPixel::TestGetLeftPixel() {
    Pixel centerPixel;
    Pixel leftPixel;

    centerPixel.SetLeftPixel(&leftPixel);

    Pixel* retrieved = centerPixel.GetLeftPixel();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(&leftPixel, retrieved);
}

void TestPixel::TestGetRightPixel() {
    Pixel centerPixel;
    Pixel rightPixel;

    centerPixel.SetRightPixel(&rightPixel);

    Pixel* retrieved = centerPixel.GetRightPixel();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(&rightPixel, retrieved);
}

void TestPixel::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestSetUpPixel);
    RUN_TEST(TestSetDownPixel);
    RUN_TEST(TestSetLeftPixel);
    RUN_TEST(TestSetRightPixel);
    RUN_TEST(TestHasUpPixel);
    RUN_TEST(TestHasDownPixel);
    RUN_TEST(TestHasLeftPixel);
    RUN_TEST(TestHasRightPixel);
    RUN_TEST(TestGetUpPixel);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetDownPixel);
    RUN_TEST(TestGetLeftPixel);
    RUN_TEST(TestGetRightPixel);
}
