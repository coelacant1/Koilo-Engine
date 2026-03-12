// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

void TestBezierQuadEvaluate();
void TestBezierQuadSplit();
void TestBezierQuadFlatten();
void TestBezierQuadBoundingBox();
void TestBezierCubicEvaluate();
void TestBezierCubicFlatten();

void TestTTFBinaryReader();
void TestTTFParserInvalidData();

void TestGlyphRasterizerEmpty();
void TestGlyphRasterizerSimple();

void TestGlyphAtlasAddFind();
void TestGlyphAtlasGrow();
void TestGlyphAtlasClear();

void TestFontUTF8Decode();
void TestFontManagerCache();

void TestDrawListSolidRect();
void TestDrawListTexturedRect();
void TestDrawListScissor();
void TestDrawListClear();
void TestDrawListSkipsInvisible();

void TestSWRendererResize();
void TestSWRendererSolidRect();
void TestSWRendererScissor();

static inline void RunFontTests() {
    RUN_TEST(TestBezierQuadEvaluate);
    RUN_TEST(TestBezierQuadSplit);
    RUN_TEST(TestBezierQuadFlatten);
    RUN_TEST(TestBezierQuadBoundingBox);
    RUN_TEST(TestBezierCubicEvaluate);
    RUN_TEST(TestBezierCubicFlatten);

    RUN_TEST(TestTTFBinaryReader);
    RUN_TEST(TestTTFParserInvalidData);

    RUN_TEST(TestGlyphRasterizerEmpty);
    RUN_TEST(TestGlyphRasterizerSimple);

    RUN_TEST(TestGlyphAtlasAddFind);
    RUN_TEST(TestGlyphAtlasGrow);
    RUN_TEST(TestGlyphAtlasClear);

    RUN_TEST(TestFontUTF8Decode);
    RUN_TEST(TestFontManagerCache);

    RUN_TEST(TestDrawListSolidRect);
    RUN_TEST(TestDrawListTexturedRect);
    RUN_TEST(TestDrawListScissor);
    RUN_TEST(TestDrawListClear);
    RUN_TEST(TestDrawListSkipsInvisible);

    RUN_TEST(TestSWRendererResize);
    RUN_TEST(TestSWRendererSolidRect);
    RUN_TEST(TestSWRendererScissor);
}
