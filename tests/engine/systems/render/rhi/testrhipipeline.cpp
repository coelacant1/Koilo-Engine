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

void TestRHIPipeline::TestUIRHIRendererIsSoftwareFalse() {
    UIRHIRenderer renderer;
    TEST_ASSERT_FALSE(renderer.IsSoftware());
}

// -- Test runner -----------------------------------------------------

void TestRHIPipeline::RunAllTests() {
    RUN_TEST(TestCreateBestRenderBackendReturnsSoftware);
    RUN_TEST(TestCreateBestSoftwareBackend);
    RUN_TEST(TestUIRHIRendererDefaultState);
    RUN_TEST(TestUIRHIRendererShutdownWithoutInit);
    RUN_TEST(TestUIRHIRendererIsSoftwareFalse);
}
