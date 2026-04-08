// SPDX-License-Identifier: GPL-3.0-or-later
#include "testklcamloader.hpp"

#include <koilo/systems/display/led/camera_layout.hpp>
#include <koilo/systems/render/core/pixelgroup.hpp>
#include <koilo/core/math/vector2d.hpp>

#include <cmath>
#include <string>

// Unity test framework
#include <unity.h>

using namespace koilo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr float kEps = 1e-4f;

static bool NearEq(float a, float b) {
    return std::fabs(a - b) < kEps;
}

// Minimal valid .klcam JSON
static const char* kMinimalKlcam = R"({
  "name": "TestLayout",
  "units": "mm",
  "count": 4,
  "positions": [
    [0.0, 0.0],
    [100.0, 0.0],
    [0.0, 50.0],
    [100.0, 50.0]
  ]
})";

// Square layout (same width and height)
static const char* kSquareKlcam = R"({
  "name": "Square",
  "units": "mm",
  "count": 4,
  "positions": [
    [10.0, 10.0],
    [110.0, 10.0],
    [10.0, 110.0],
    [110.0, 110.0]
  ]
})";

// Single point layout
static const char* kSinglePoint = R"({
  "name": "SingleLED",
  "units": "mm",
  "count": 1,
  "positions": [[42.0, 17.0]]
})";

// ProtoDR-like excerpt (non-zero minimum coords)
static const char* kOffsetKlcam = R"({
  "name": "OffsetTest",
  "units": "mm",
  "count": 3,
  "positions": [
    [20.0, 5.0],
    [27.0, 5.0],
    [20.0, 12.0]
  ]
})";

// ---------------------------------------------------------------------------
// Tests: Parsing
// ---------------------------------------------------------------------------

static void TestKlcamLoadFromStringBasic() {
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kMinimalKlcam));
    TEST_ASSERT_EQUAL_STRING("TestLayout", layout.GetName().c_str());
    TEST_ASSERT_EQUAL_UINT32(4, layout.GetCount());
    TEST_ASSERT_TRUE(layout.IsLoaded());
}

static void TestKlcamLoadFromStringEmpty() {
    LEDCameraLayout layout;
    TEST_ASSERT_FALSE(layout.LoadFromString(""));
    TEST_ASSERT_FALSE(layout.IsLoaded());
}

static void TestKlcamLoadFromStringInvalidJSON() {
    LEDCameraLayout layout;
    TEST_ASSERT_FALSE(layout.LoadFromString("{not valid json"));
}

static void TestKlcamLoadFromStringNoPositions() {
    LEDCameraLayout layout;
    TEST_ASSERT_FALSE(layout.LoadFromString(R"({"name": "Empty", "count": 0, "positions": []})"));
}

static void TestKlcamLoadFromStringMissingPositionsKey() {
    LEDCameraLayout layout;
    TEST_ASSERT_FALSE(layout.LoadFromString(R"({"name": "Bad", "count": 1})"));
}

// ---------------------------------------------------------------------------
// Tests: Normalization
// ---------------------------------------------------------------------------

static void TestKlcamNormalizationRectangular() {
    // 100mm wide x 50mm tall -> normExtent = 100 (max)
    // Stretch-to-fill with V-flip: X/width, 1-(Y/height)
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kMinimalKlcam));

    TEST_ASSERT_TRUE(NearEq(100.0f, layout.GetNormalizationExtent()));
    TEST_ASSERT_TRUE(NearEq(100.0f, layout.GetPhysicalSize().X));
    TEST_ASSERT_TRUE(NearEq(50.0f, layout.GetPhysicalSize().Y));

    const Vector2D* uv = layout.GetNormalizedPositions();
    TEST_ASSERT_NOT_NULL(uv);

    // (0,0) -> u=0, v=1-0=1
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[0].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[0].Y));
    // (100,0) -> u=1, v=1-0=1
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[1].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[1].Y));
    // (0,50) -> u=0, v=1-1=0
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].X));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].Y));
    // (100,50) -> u=1, v=1-1=0
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[3].X));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[3].Y));
}

static void TestKlcamNormalizationSquare() {
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kSquareKlcam));

    TEST_ASSERT_TRUE(NearEq(100.0f, layout.GetNormalizationExtent()));

    const Vector2D* uv = layout.GetNormalizedPositions();
    TEST_ASSERT_NOT_NULL(uv);

    TEST_ASSERT_TRUE(NearEq(0.0f, uv[0].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[0].Y));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[1].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[1].Y));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].X));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].Y));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[3].X));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[3].Y));
}

static void TestKlcamNormalizationWithOffset() {
    // Points at (20,5), (27,5), (20,12)
    // Bounding box: 7mm x 7mm -> normExtent = 7
    // V-flipped: (20,5)->(0,1), (27,5)->(1,1), (20,12)->(0,0)
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kOffsetKlcam));

    TEST_ASSERT_TRUE(NearEq(7.0f, layout.GetNormalizationExtent()));

    const Vector2D* uv = layout.GetNormalizedPositions();
    TEST_ASSERT_NOT_NULL(uv);

    TEST_ASSERT_TRUE(NearEq(0.0f, uv[0].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[0].Y));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[1].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[1].Y));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].X));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].Y));
}

static void TestKlcamNormalizationSinglePoint() {
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kSinglePoint));

    TEST_ASSERT_EQUAL_UINT32(1, layout.GetCount());
    TEST_ASSERT_TRUE(NearEq(0.0f, layout.GetNormalizationExtent()));

    const Vector2D* uv = layout.GetNormalizedPositions();
    TEST_ASSERT_NOT_NULL(uv);
    TEST_ASSERT_TRUE(NearEq(0.5f, uv[0].X));
    TEST_ASSERT_TRUE(NearEq(0.5f, uv[0].Y));
}

static void TestKlcamNormalizationPreservesAspectRatio() {
    // 100mm wide x 50mm tall -> stretch-to-fill, max UV.Y should be 1.0.
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kMinimalKlcam));

    const Vector2D* uv = layout.GetNormalizedPositions();
    float maxY = 0;
    for (uint32_t i = 0; i < layout.GetCount(); ++i) {
        if (uv[i].Y > maxY) maxY = uv[i].Y;
    }
    TEST_ASSERT_TRUE(NearEq(1.0f, maxY));
}

// ---------------------------------------------------------------------------
// Tests: Raw positions
// ---------------------------------------------------------------------------

static void TestKlcamRawPositionsPreserved() {
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kMinimalKlcam));

    const Vector2D* raw = layout.GetRawPositions();
    TEST_ASSERT_NOT_NULL(raw);
    TEST_ASSERT_TRUE(NearEq(0.0f, raw[0].X));
    TEST_ASSERT_TRUE(NearEq(0.0f, raw[0].Y));
    TEST_ASSERT_TRUE(NearEq(100.0f, raw[1].X));
    TEST_ASSERT_TRUE(NearEq(50.0f, raw[3].Y));
}

// ---------------------------------------------------------------------------
// Tests: PixelGroup creation
// ---------------------------------------------------------------------------

static void TestKlcamCreatePixelGroup() {
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kMinimalKlcam));

    PixelGroup* pg = layout.CreatePixelGroup();
    TEST_ASSERT_NOT_NULL(pg);
    TEST_ASSERT_EQUAL_UINT32(4, pg->GetPixelCount());

    // (0,0) -> (0, 1.0) with V-flip
    Vector2D coord = pg->GetCoordinate(0);
    TEST_ASSERT_TRUE(NearEq(0.0f, coord.X));
    TEST_ASSERT_TRUE(NearEq(1.0f, coord.Y));

    // (100,50) -> (1.0, 0.0) with V-flip
    Vector2D coord3 = pg->GetCoordinate(3);
    TEST_ASSERT_TRUE(NearEq(1.0f, coord3.X));
    TEST_ASSERT_TRUE(NearEq(0.0f, coord3.Y));

    delete pg;
}

static void TestKlcamCreatePixelGroupWhenEmpty() {
    LEDCameraLayout layout;
    PixelGroup* pg = layout.CreatePixelGroup();
    TEST_ASSERT_NULL(pg);
}

// ---------------------------------------------------------------------------
// Tests: Reload / overwrite
// ---------------------------------------------------------------------------

static void TestKlcamReloadOverwritesPrevious() {
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(kMinimalKlcam));
    TEST_ASSERT_EQUAL_UINT32(4, layout.GetCount());

    TEST_ASSERT_TRUE(layout.LoadFromString(kOffsetKlcam));
    TEST_ASSERT_EQUAL_UINT32(3, layout.GetCount());
    TEST_ASSERT_EQUAL_STRING("OffsetTest", layout.GetName().c_str());
}

// ---------------------------------------------------------------------------
// Tests: Edge cases
// ---------------------------------------------------------------------------

static void TestKlcamCollinearHorizontal() {
    const char* json = R"({
      "name": "HLine",
      "units": "mm",
      "count": 3,
      "positions": [[0,5],[50,5],[100,5]]
    })";
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(json));
    TEST_ASSERT_TRUE(NearEq(100.0f, layout.GetNormalizationExtent()));

    const Vector2D* uv = layout.GetNormalizedPositions();
    // All same Y, height=0 -> scaleY fallback=1 -> V-flip: 1.0
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[0].Y));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[2].Y));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[0].X));
    TEST_ASSERT_TRUE(NearEq(0.5f, uv[1].X));
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[2].X));
}

static void TestKlcamCollinearVertical() {
    const char* json = R"({
      "name": "VLine",
      "units": "mm",
      "count": 3,
      "positions": [[10,0],[10,50],[10,100]]
    })";
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(json));
    TEST_ASSERT_TRUE(NearEq(100.0f, layout.GetNormalizationExtent()));

    const Vector2D* uv = layout.GetNormalizedPositions();
    // All same X, width=0 -> scaleX fallback=1 -> 0
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[0].X));
    // V-flipped: Y=0 -> 1.0, Y=50 -> 0.5, Y=100 -> 0.0
    TEST_ASSERT_TRUE(NearEq(1.0f, uv[0].Y));
    TEST_ASSERT_TRUE(NearEq(0.5f, uv[1].Y));
    TEST_ASSERT_TRUE(NearEq(0.0f, uv[2].Y));
}

static void TestKlcamUnknownFieldsIgnored() {
    const char* json = R"({
      "name": "WithExtra",
      "units": "mm",
      "version": 2,
      "author": "test",
      "count": 2,
      "positions": [[0,0],[10,10]]
    })";
    LEDCameraLayout layout;
    TEST_ASSERT_TRUE(layout.LoadFromString(json));
    TEST_ASSERT_EQUAL_UINT32(2, layout.GetCount());
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

void RunTestKlcamLoader() {
    RUN_TEST(TestKlcamLoadFromStringBasic);
    RUN_TEST(TestKlcamLoadFromStringEmpty);
    RUN_TEST(TestKlcamLoadFromStringInvalidJSON);
    RUN_TEST(TestKlcamLoadFromStringNoPositions);
    RUN_TEST(TestKlcamLoadFromStringMissingPositionsKey);

    RUN_TEST(TestKlcamNormalizationRectangular);
    RUN_TEST(TestKlcamNormalizationSquare);
    RUN_TEST(TestKlcamNormalizationWithOffset);
    RUN_TEST(TestKlcamNormalizationSinglePoint);
    RUN_TEST(TestKlcamNormalizationPreservesAspectRatio);

    RUN_TEST(TestKlcamRawPositionsPreserved);

    RUN_TEST(TestKlcamCreatePixelGroup);
    RUN_TEST(TestKlcamCreatePixelGroupWhenEmpty);

    RUN_TEST(TestKlcamReloadOverwritesPrevious);

    RUN_TEST(TestKlcamCollinearHorizontal);
    RUN_TEST(TestKlcamCollinearVertical);
    RUN_TEST(TestKlcamUnknownFieldsIgnored);
}
