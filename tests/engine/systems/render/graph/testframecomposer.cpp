// SPDX-License-Identifier: GPL-3.0-or-later
#include "testframecomposer.hpp"
#include <koilo/systems/render/graph/frame_composer.hpp>
#include <vector>
#include <string>

using namespace koilo::rhi;

namespace TestFrameComposer {

void TestRegisterAndListProviders() {
    FrameComposer c;
    c.RegisterProvider("a", PassPhase::Scene,   [](RenderGraph&, const FrameContext&) {});
    c.RegisterProvider("b", PassPhase::Overlay,  [](RenderGraph&, const FrameContext&) {});
    c.RegisterProvider("c", PassPhase::Compose,  [](RenderGraph&, const FrameContext&) {});

    auto& providers = c.GetProviders();
    TEST_ASSERT_EQUAL_INT(3, (int)providers.size());
    TEST_ASSERT_EQUAL_STRING("a", providers[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("b", providers[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("c", providers[2].name.c_str());
}

void TestPhaseOrdering() {
    FrameComposer c;
    // Register out of order
    c.RegisterProvider("ui",      PassPhase::Overlay, [](RenderGraph& g, const FrameContext&) {
        g.AddPass("ui", {}, {"out"}, []() {});
    });
    c.RegisterProvider("compose", PassPhase::Compose, [](RenderGraph& g, const FrameContext&) {
        g.AddPass("compose", {}, {"mid"}, []() {});
    });
    c.RegisterProvider("scene",   PassPhase::Scene,   [](RenderGraph& g, const FrameContext&) {
        g.AddPass("scene", {}, {"fb"}, []() {});
    });

    FrameContext ctx{};
    TEST_ASSERT_TRUE(c.BuildAndExecute(ctx));

    auto order = c.GetExecutionOrder();
    TEST_ASSERT_EQUAL_INT(3, (int)order.size());
    // Phase sort: Scene < Compose < Overlay
    TEST_ASSERT_EQUAL_STRING("scene",   order[0].c_str());
    TEST_ASSERT_EQUAL_STRING("compose", order[1].c_str());
    TEST_ASSERT_EQUAL_STRING("ui",      order[2].c_str());
}

void TestEnableDisableProvider() {
    FrameComposer c;
    c.RegisterProvider("a", PassPhase::Scene, [](RenderGraph&, const FrameContext&) {});
    TEST_ASSERT_TRUE(c.IsProviderEnabled("a"));

    c.SetProviderEnabled("a", false);
    TEST_ASSERT_FALSE(c.IsProviderEnabled("a"));

    c.SetProviderEnabled("a", true);
    TEST_ASSERT_TRUE(c.IsProviderEnabled("a"));
}

void TestUnregisterProvider() {
    FrameComposer c;
    c.RegisterProvider("a", PassPhase::Scene,   [](RenderGraph&, const FrameContext&) {});
    c.RegisterProvider("b", PassPhase::Overlay,  [](RenderGraph&, const FrameContext&) {});
    TEST_ASSERT_EQUAL_INT(2, (int)c.GetProviders().size());

    c.UnregisterProvider("a");
    TEST_ASSERT_EQUAL_INT(1, (int)c.GetProviders().size());
    TEST_ASSERT_EQUAL_STRING("b", c.GetProviders()[0].name.c_str());
}

void TestBuildAndExecuteCallsProviders() {
    FrameComposer c;
    int callCount = 0;
    c.RegisterProvider("test", PassPhase::Scene,
        [&callCount](RenderGraph& g, const FrameContext&) {
            ++callCount;
            g.AddPass("p", {}, {"out"}, []() {});
        });

    FrameContext ctx{};
    TEST_ASSERT_TRUE(c.BuildAndExecute(ctx));
    TEST_ASSERT_EQUAL_INT(1, callCount);

    // Second frame: providers called again
    TEST_ASSERT_TRUE(c.BuildAndExecute(ctx));
    TEST_ASSERT_EQUAL_INT(2, callCount);
}

void TestDisabledProviderSkipped() {
    FrameComposer c;
    int aCount = 0, bCount = 0;
    c.RegisterProvider("a", PassPhase::Scene,
        [&aCount](RenderGraph& g, const FrameContext&) {
            ++aCount;
            g.AddPass("pa", {}, {"x"}, []() {});
        });
    c.RegisterProvider("b", PassPhase::Compose,
        [&bCount](RenderGraph& g, const FrameContext&) {
            ++bCount;
            g.AddPass("pb", {}, {"y"}, []() {});
        });

    c.SetProviderEnabled("a", false);
    FrameContext ctx{};
    c.BuildAndExecute(ctx);

    TEST_ASSERT_EQUAL_INT(0, aCount);
    TEST_ASSERT_EQUAL_INT(1, bCount);
}

void TestReplaceExistingProvider() {
    FrameComposer c;
    int v1 = 0, v2 = 0;
    c.RegisterProvider("x", PassPhase::Scene,
        [&v1](RenderGraph&, const FrameContext&) { ++v1; });
    c.RegisterProvider("x", PassPhase::Compose,
        [&v2](RenderGraph&, const FrameContext&) { ++v2; });

    // Should have replaced, not duplicated
    TEST_ASSERT_EQUAL_INT(1, (int)c.GetProviders().size());
    TEST_ASSERT_EQUAL_STRING("x", c.GetProviders()[0].name.c_str());

    FrameContext ctx{};
    c.BuildAndExecute(ctx);
    TEST_ASSERT_EQUAL_INT(0, v1);  // Old callback not called
    TEST_ASSERT_EQUAL_INT(1, v2);  // New callback called
}

void TestExecutionOrderQuery() {
    FrameComposer c;
    c.RegisterProvider("scene", PassPhase::Scene, [](RenderGraph& g, const FrameContext&) {
        g.AddPass("scene_begin", {}, {"offscreen"}, []() {});
        g.AddPass("scene_end",   {"offscreen"}, {"offscreen"}, []() {});
    });
    c.RegisterProvider("blit", PassPhase::Compose, [](RenderGraph& g, const FrameContext&) {
        g.AddPass("blit", {"offscreen"}, {"swapchain"}, []() {});
    });

    FrameContext ctx{};
    c.BuildAndExecute(ctx);

    auto order = c.GetExecutionOrder();
    TEST_ASSERT_EQUAL_INT(3, (int)order.size());
    TEST_ASSERT_EQUAL_STRING("scene_begin", order[0].c_str());
    TEST_ASSERT_EQUAL_STRING("scene_end",   order[1].c_str());
    TEST_ASSERT_EQUAL_STRING("blit",        order[2].c_str());
}

void RunAllTests() {
    RUN_TEST(TestRegisterAndListProviders);
    RUN_TEST(TestPhaseOrdering);
    RUN_TEST(TestEnableDisableProvider);
    RUN_TEST(TestUnregisterProvider);
    RUN_TEST(TestBuildAndExecuteCallsProviders);
    RUN_TEST(TestDisabledProviderSkipped);
    RUN_TEST(TestReplaceExistingProvider);
    RUN_TEST(TestExecutionOrderQuery);
}

} // namespace TestFrameComposer
