// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrhipipeline.cpp
 * @brief RHI pipeline and renderer smoke tests.
 *
 * Validates the IUIRenderer contract, backend factory, and renderer
 * lifecycle without requiring a live GPU context.
 */
#include "testrhipipeline.hpp"
#include <koilo/systems/ui/render/ui_renderer.hpp>
#include <koilo/systems/ui/render/ui_rhi_renderer.hpp>
#include <koilo/systems/ui/render/ui_sw_renderer.hpp>
#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/render/gl/render_backend_factory.hpp>
#include <koilo/systems/render/irenderbackend.hpp>
#include <memory>

using namespace koilo;
using namespace koilo::ui;

// -- Factory tests ---------------------------------------------------

void TestRHIPipeline::TestCreateBestRenderBackendReturnsSoftware() {
    // Without a GL context, factory should return software backend
    auto backend = CreateBestRenderBackend();
    TEST_ASSERT_NOT_NULL(backend.get());
    TEST_ASSERT_TRUE(backend->IsInitialized());
}

void TestRHIPipeline::TestCreateBestSoftwareBackend() {
    auto backend = CreateBestSoftwareBackend();
    TEST_ASSERT_NOT_NULL(backend.get());
    TEST_ASSERT_TRUE(backend->IsInitialized());
    const char* name = backend->GetName();
    TEST_ASSERT_NOT_NULL(name);
}

// -- UIRHIRenderer (no device) tests ---------------------------------

void TestRHIPipeline::TestUIRHIRendererDefaultState() {
    UIRHIRenderer renderer;
    TEST_ASSERT_FALSE(renderer.IsInitialized());
    TEST_ASSERT_FALSE(renderer.IsSoftware());
    TEST_ASSERT_NULL(renderer.Pixels());
}

void TestRHIPipeline::TestUIRHIRendererShutdownWithoutInit() {
    UIRHIRenderer renderer;
    // Shutdown on uninitialized renderer should not crash
    renderer.Shutdown();
    TEST_ASSERT_FALSE(renderer.IsInitialized());
}

// -- UISWRenderer IUIRenderer interface tests ------------------------

void TestRHIPipeline::TestUISWRendererIsInitialized() {
    UISWRenderer renderer;
    // SW renderer is always "initialized"
    TEST_ASSERT_TRUE(renderer.IsInitialized());
}

void TestRHIPipeline::TestUISWRendererIsSoftware() {
    UISWRenderer renderer;
    TEST_ASSERT_TRUE(renderer.IsSoftware());
}

void TestRHIPipeline::TestUIRHIRendererIsSoftwareFalse() {
    UIRHIRenderer renderer;
    TEST_ASSERT_FALSE(renderer.IsSoftware());
}

void TestRHIPipeline::TestUISWRendererSetFont() {
    UISWRenderer renderer;
    // SetFont with nullptr returns 0
    uint32_t handle = renderer.SetFont(nullptr);
    TEST_ASSERT_EQUAL_UINT32(0, handle);

    // SetFont with a valid font returns sentinel 1
    font::Font font;
    font.LoadFromFile("assets/fonts/NotoSansMono-VariableFont_wdth,wght.ttf", 16.0f);
    if (font.IsLoaded()) {
        handle = renderer.SetFont(&font);
        TEST_ASSERT_EQUAL_UINT32(1, handle);
    }
}

void TestRHIPipeline::TestUISWRendererSetBoldFont() {
    UISWRenderer renderer;
    uint32_t handle = renderer.SetBoldFont(nullptr);
    TEST_ASSERT_EQUAL_UINT32(0, handle);

    font::Font bold;
    bold.LoadFromFile("assets/fonts/NotoSansMono-Bold.ttf", 16.0f);
    if (bold.IsLoaded()) {
        handle = renderer.SetBoldFont(&bold);
        TEST_ASSERT_EQUAL_UINT32(2, handle);
    }
}

void TestRHIPipeline::TestUISWRendererRenderEmptyDrawList() {
    UISWRenderer renderer;
    UIDrawList dl;
    // Render with empty draw list should not crash
    renderer.Render(dl, 64, 64);
    const uint8_t* pixels = renderer.Pixels();
    TEST_ASSERT_NOT_NULL(pixels);
    TEST_ASSERT_EQUAL(64, renderer.Width());
    TEST_ASSERT_EQUAL(64, renderer.Height());
}

void TestRHIPipeline::TestUISWRendererRenderProducesPixels() {
    UISWRenderer renderer;
    UIDrawList dl;
    dl.AddSolidRect(10.0f, 10.0f, 20.0f, 20.0f, Color4(255, 0, 0, 255));
    renderer.Render(dl, 64, 64);

    const uint8_t* pixels = renderer.Pixels();
    TEST_ASSERT_NOT_NULL(pixels);

    // Pixel at (20, 20) should have red contribution
    int idx = (20 * 64 + 20) * 4;
    TEST_ASSERT_EQUAL(255, pixels[idx + 0]); // R
    TEST_ASSERT_EQUAL(0,   pixels[idx + 1]); // G
    TEST_ASSERT_EQUAL(0,   pixels[idx + 2]); // B
    TEST_ASSERT_EQUAL(255, pixels[idx + 3]); // A
}

void TestRHIPipeline::TestUISWRendererShutdownClearsState() {
    UISWRenderer renderer;
    UIDrawList dl;
    renderer.Render(dl, 64, 64);
    TEST_ASSERT_NOT_NULL(renderer.Pixels());
    TEST_ASSERT_EQUAL(64, renderer.Width());

    renderer.Shutdown();
    TEST_ASSERT_EQUAL(0, renderer.Width());
    TEST_ASSERT_EQUAL(0, renderer.Height());
}

void TestRHIPipeline::TestUISWRendererSyncFontAtlases() {
    UISWRenderer renderer;
    uint32_t fh = 1, bh = 2;
    // SyncFontAtlases with nulls should not crash
    renderer.SyncFontAtlases(nullptr, fh, nullptr, bh);
    TEST_ASSERT_EQUAL_UINT32(1, fh);
    TEST_ASSERT_EQUAL_UINT32(2, bh);
}

// -- Polymorphism test -----------------------------------------------

static void VerifyInterface(IUIRenderer& r, bool expectSW) {
    TEST_ASSERT_EQUAL(expectSW, r.IsSoftware());
    if (expectSW) {
        TEST_ASSERT_TRUE(r.IsInitialized());
    }
}

// -- Test runner -----------------------------------------------------

void TestRHIPipeline::RunAllTests() {
    RUN_TEST(TestCreateBestRenderBackendReturnsSoftware);
    RUN_TEST(TestCreateBestSoftwareBackend);
    RUN_TEST(TestUIRHIRendererDefaultState);
    RUN_TEST(TestUIRHIRendererShutdownWithoutInit);
    RUN_TEST(TestUISWRendererIsInitialized);
    RUN_TEST(TestUISWRendererIsSoftware);
    RUN_TEST(TestUIRHIRendererIsSoftwareFalse);
    RUN_TEST(TestUISWRendererSetFont);
    RUN_TEST(TestUISWRendererSetBoldFont);
    RUN_TEST(TestUISWRendererRenderEmptyDrawList);
    RUN_TEST(TestUISWRendererRenderProducesPixels);
    RUN_TEST(TestUISWRendererShutdownClearsState);
    RUN_TEST(TestUISWRendererSyncFontAtlases);

    // Polymorphism check
    UISWRenderer sw;
    VerifyInterface(sw, true);
    UIRHIRenderer rhi;
    VerifyInterface(rhi, false);
}
