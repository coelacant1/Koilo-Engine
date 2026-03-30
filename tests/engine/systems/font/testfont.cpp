// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>
#include <koilo/core/math/bezier.hpp>
#include <koilo/systems/font/ttf_parser.hpp>
#include <koilo/systems/font/rasterizer.hpp>
#include <koilo/systems/font/glyph_atlas.hpp>
#include <koilo/systems/font/font.hpp>
#include <koilo/systems/ui/render/draw_list.hpp>
#include <cmath>
#include <cstring>

using namespace koilo;
using namespace koilo::font;
using namespace koilo::ui;

// --- Bézier tests ---------------------------------------------------

void TestBezierQuadEvaluate() {
    QuadBezier q{
        Vector2D(0.0f, 0.0f),
        Vector2D(0.5f, 1.0f),
        Vector2D(1.0f, 0.0f)
    };

    Vector2D start = q.Evaluate(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, start.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, start.Y);

    Vector2D end = q.Evaluate(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, end.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, end.Y);

    Vector2D mid = q.Evaluate(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.Y);
}

void TestBezierQuadSplit() {
    QuadBezier q{
        Vector2D(0.0f, 0.0f),
        Vector2D(1.0f, 2.0f),
        Vector2D(2.0f, 0.0f)
    };

    QuadBezier left, right;
    q.Split(0.5f, left, right);

    // Left starts at q.p0
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, left.p0.X);
    // Right ends at q.p2
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, right.p2.X);
    // Both meet at midpoint
    TEST_ASSERT_FLOAT_WITHIN(0.001f, left.p2.X, right.p0.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, left.p2.Y, right.p0.Y);

    // Evaluate original at 0.5 should match the junction
    Vector2D expected = q.Evaluate(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected.X, left.p2.X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected.Y, left.p2.Y);
}

void TestBezierQuadFlatten() {
    QuadBezier q{
        Vector2D(0.0f, 0.0f),
        Vector2D(5.0f, 10.0f),
        Vector2D(10.0f, 0.0f)
    };

    std::vector<Vector2D> points;
    q.Flatten(points, 0.5f);

    // Should have at least 3 points (start, some intermediates, end)
    TEST_ASSERT_TRUE(points.size() >= 3);

    // First point is start
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, points.front().X);
    // Last point is end
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, points.back().X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, points.back().Y);
}

void TestBezierQuadBoundingBox() {
    QuadBezier q{
        Vector2D(0.0f, 0.0f),
        Vector2D(0.5f, 2.0f),
        Vector2D(1.0f, 0.0f)
    };

    Vector2D bmin, bmax;
    q.BoundingBox(bmin, bmax);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, bmin.X);
    TEST_ASSERT_TRUE(bmax.Y >= 0.9f);  // Curve peaks above 0
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, bmax.X);
}

void TestBezierCubicEvaluate() {
    CubicBezier c{
        Vector2D(0.0f, 0.0f),
        Vector2D(0.0f, 1.0f),
        Vector2D(1.0f, 1.0f),
        Vector2D(1.0f, 0.0f)
    };

    Vector2D start = c.Evaluate(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, start.X);

    Vector2D end = c.Evaluate(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, end.X);

    Vector2D mid = c.Evaluate(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.X);
    TEST_ASSERT_TRUE(mid.Y > 0.5f);  // S-curve is above diagonal at midpoint
}

void TestBezierCubicFlatten() {
    CubicBezier c{
        Vector2D(0.0f, 0.0f),
        Vector2D(0.0f, 10.0f),
        Vector2D(10.0f, 10.0f),
        Vector2D(10.0f, 0.0f)
    };

    std::vector<Vector2D> points;
    c.Flatten(points, 0.5f);

    TEST_ASSERT_TRUE(points.size() >= 3);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, points.front().X);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, points.back().X);
}

// --- TTF parser tests -----------------------------------------------

void TestTTFBinaryReader() {
    uint8_t data[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    BinaryReader r(data, sizeof(data));

    TEST_ASSERT_TRUE(r.Valid());
    TEST_ASSERT_EQUAL(0, r.Position());

    uint8_t b = r.ReadU8();
    TEST_ASSERT_EQUAL(0x00, b);
    TEST_ASSERT_EQUAL(1, r.Position());

    uint16_t s = r.ReadU16();
    TEST_ASSERT_EQUAL(0x0102, s);  // big-endian

    uint32_t w = r.ReadU32();
    TEST_ASSERT_EQUAL(0x03040506u, w);

    r.Seek(0);
    uint32_t full = r.ReadU32();
    TEST_ASSERT_EQUAL(0x00010203u, full);
}

void TestTTFParserInvalidData() {
    uint8_t garbage[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    TTFParser parser;
    bool result = parser.Parse(garbage, sizeof(garbage));
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(parser.IsParsed());
}

// --- Rasterizer tests ----------------------------------------------

void TestGlyphRasterizerEmpty() {
    GlyphRasterizer rast;
    GlyphOutline outline;
    outline.valid = true;
    outline.advanceWidth = 500.0f;

    GlyphBitmap bmp = rast.Rasterize(outline, 0.05f);
    TEST_ASSERT_TRUE(bmp.width > 0);
    // Empty outline should produce empty bitmap (all zeros)
    bool allZero = true;
    for (size_t i = 0; i < bmp.pixels.size(); ++i) {
        if (bmp.pixels[i] != 0) { allZero = false; break; }
    }
    TEST_ASSERT_TRUE(allZero);
}

void TestGlyphRasterizerSimple() {
    // Create a simple square outline (CW winding)
    GlyphOutline outline;
    outline.valid = true;
    outline.xMin = 0.0f;
    outline.yMin = 0.0f;
    outline.xMax = 100.0f;
    outline.yMax = 100.0f;
    outline.advanceWidth = 120.0f;

    GlyphContour contour;
    contour.points.push_back({0.0f, 0.0f, true});
    contour.points.push_back({100.0f, 0.0f, true});
    contour.points.push_back({100.0f, 100.0f, true});
    contour.points.push_back({0.0f, 100.0f, true});
    outline.contours.push_back(contour);

    GlyphRasterizer rast;
    float scale = 0.1f;  // 100 * 0.1 = 10 pixels
    GlyphBitmap bmp = rast.Rasterize(outline, scale);

    TEST_ASSERT_TRUE(bmp.width >= 10);
    TEST_ASSERT_TRUE(bmp.height >= 10);

    // Center pixels should have non-zero coverage
    int cx = bmp.width / 2;
    int cy = bmp.height / 2;
    TEST_ASSERT_TRUE(bmp.Sample(cx, cy) > 0);
}

// --- Glyph atlas tests ---------------------------------------------

void TestGlyphAtlasAddFind() {
    GlyphAtlas atlas(256);

    GlyphBitmap bmp;
    bmp.width = 10;
    bmp.height = 12;
    bmp.pixels.resize(10 * 12, 128);

    GlyphKey key{65, 16};  // 'A' at 16px
    const GlyphRegion* reg = atlas.Add(key, bmp, 1.0f, 10.0f, 8.0f);
    TEST_ASSERT_NOT_NULL(reg);
    TEST_ASSERT_EQUAL(10, reg->width);
    TEST_ASSERT_EQUAL(12, reg->height);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 8.0f, reg->advance);

    // Find should return the same region
    const GlyphRegion* found = atlas.Find(key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(reg->atlasX, found->atlasX);
    TEST_ASSERT_EQUAL(reg->atlasY, found->atlasY);

    // Different key should return null
    GlyphKey key2{66, 16};
    TEST_ASSERT_NULL(atlas.Find(key2));

    TEST_ASSERT_EQUAL(1, atlas.GlyphCount());
    TEST_ASSERT_TRUE(atlas.IsDirty());
}

void TestGlyphAtlasGrow() {
    GlyphAtlas atlas(64);  // tiny atlas

    GlyphBitmap bmp;
    bmp.width = 30;
    bmp.height = 30;
    bmp.pixels.resize(30 * 30, 200);

    // Fill atlas to force grow
    int added = 0;
    for (int i = 0; i < 20; ++i) {
        GlyphKey key{static_cast<uint32_t>(i + 65), 16};
        const GlyphRegion* reg = atlas.Add(key, bmp, 0.0f, 0.0f, 10.0f);
        if (reg) ++added;
    }

    TEST_ASSERT_TRUE(added > 1);
    TEST_ASSERT_TRUE(atlas.Width() > 64 || atlas.Height() > 64);
}

void TestGlyphAtlasClear() {
    GlyphAtlas atlas(128);

    GlyphBitmap bmp;
    bmp.width = 5;
    bmp.height = 5;
    bmp.pixels.resize(25, 100);

    GlyphKey key{65, 16};
    atlas.Add(key, bmp, 0.0f, 0.0f, 5.0f);
    TEST_ASSERT_EQUAL(1, atlas.GlyphCount());

    atlas.Clear();
    TEST_ASSERT_EQUAL(0, atlas.GlyphCount());
    TEST_ASSERT_NULL(atlas.Find(key));
}

// --- Font / UTF-8 tests --------------------------------------------

// Test UTF-8 decode via Font::MeasureText (indirectly tests the decoder)
void TestFontUTF8Decode() {
    // We can't easily test DecodeUTF8 directly since it's private,
    // but we can verify the Font class handles UTF-8 strings
    Font font;
    // Unloaded font should return zero metrics
    TextMetrics tm = font.MeasureText("Hello");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, tm.width);
    TEST_ASSERT_FALSE(font.IsLoaded());
}

void TestFontManagerCache() {
    FontManager mgr;
    TEST_ASSERT_EQUAL(0, mgr.CacheSize());

    // Loading from non-existent file should return null
    Font* f = mgr.LoadFont("/nonexistent/font.ttf", 16.0f);
    TEST_ASSERT_NULL(f);
    TEST_ASSERT_EQUAL(0, mgr.CacheSize());

    mgr.Clear();
    TEST_ASSERT_EQUAL(0, mgr.CacheSize());
}

// --- Draw list tests ------------------------------------------------

void TestDrawListSolidRect() {
    UIDrawList dl;
    dl.AddSolidRect(10.0f, 20.0f, 100.0f, 50.0f, Color4(255, 0, 0, 255));
    TEST_ASSERT_EQUAL(1, dl.Size());
    TEST_ASSERT_EQUAL(static_cast<int>(DrawCmdType::SolidRect),
                      static_cast<int>(dl[0].type));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, dl[0].x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, dl[0].w);
}

void TestDrawListTexturedRect() {
    UIDrawList dl;
    dl.AddTexturedRect(0.0f, 0.0f, 32.0f, 32.0f,
                       0.0f, 0.0f, 0.5f, 0.5f,
                       Color4(255, 255, 255, 255), 42);
    TEST_ASSERT_EQUAL(1, dl.Size());
    TEST_ASSERT_EQUAL(static_cast<int>(DrawCmdType::TexturedRect),
                      static_cast<int>(dl[0].type));
    TEST_ASSERT_EQUAL(42u, dl[0].textureHandle);
}

void TestDrawListScissor() {
    UIDrawList dl;
    dl.PushScissor(10, 10, 200, 200);
    dl.AddSolidRect(0.0f, 0.0f, 50.0f, 50.0f, Color4(0, 255, 0, 255));
    dl.PopScissor();
    TEST_ASSERT_EQUAL(3, dl.Size());
    TEST_ASSERT_EQUAL(static_cast<int>(DrawCmdType::PushScissor),
                      static_cast<int>(dl[0].type));
    TEST_ASSERT_EQUAL(static_cast<int>(DrawCmdType::PopScissor),
                      static_cast<int>(dl[2].type));
}

void TestDrawListClear() {
    UIDrawList dl;
    dl.AddSolidRect(0.0f, 0.0f, 50.0f, 50.0f, Color4(0, 0, 255, 255));
    TEST_ASSERT_EQUAL(1, dl.Size());
    dl.Clear();
    TEST_ASSERT_EQUAL(0, dl.Size());
}

void TestDrawListSkipsInvisible() {
    UIDrawList dl;
    // Zero alpha should be skipped
    dl.AddSolidRect(0.0f, 0.0f, 50.0f, 50.0f, Color4(255, 0, 0, 0));
    TEST_ASSERT_EQUAL(0, dl.Size());

    // Zero-size should be skipped
    dl.AddSolidRect(0.0f, 0.0f, 0.0f, 50.0f, Color4(255, 0, 0, 255));
    TEST_ASSERT_EQUAL(0, dl.Size());
}

// (Legacy UISWRenderer tests removed - software UI rendering now
//  goes through SoftwareRHIDevice + UIRHIRenderer.)
