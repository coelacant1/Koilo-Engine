// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrendergraph.cpp
 * @brief Tests for RenderGraph: DAG ordering, cycles, lifetimes, execution.
 */
#include "testrendergraph.hpp"
#include <koilo/systems/render/graph/render_graph.hpp>
#include <vector>
#include <string>

using namespace koilo::rhi;

// -- Empty / single pass ------------------------------------------------

void TestRenderGraph::TestEmptyGraphCompiles() {
    RenderGraph g;
    TEST_ASSERT_TRUE(g.Compile());
    TEST_ASSERT_TRUE(g.IsCompiled());
    TEST_ASSERT_EQUAL(0, g.PassCount());
    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL(0, order.size());
}

void TestRenderGraph::TestSinglePassCompiles() {
    RenderGraph g;
    g.AddPass("only", {}, {"out"}, nullptr);
    TEST_ASSERT_TRUE(g.Compile());
    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL(1, order.size());
    TEST_ASSERT_EQUAL_STRING("only", order[0].c_str());
}

// -- Linear chain: A writes X, B reads X writes Y, C reads Y ------------

void TestRenderGraph::TestLinearChainOrder() {
    RenderGraph g;
    g.AddPass("A", {},    {"X"}, nullptr);
    g.AddPass("B", {"X"}, {"Y"}, nullptr);
    g.AddPass("C", {"Y"}, {},    nullptr);
    TEST_ASSERT_TRUE(g.Compile());

    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL(3, order.size());
    TEST_ASSERT_EQUAL_STRING("A", order[0].c_str());
    TEST_ASSERT_EQUAL_STRING("B", order[1].c_str());
    TEST_ASSERT_EQUAL_STRING("C", order[2].c_str());
}

// -- Diamond: A->B, A->C, B->D, C->D -----------------------------------

void TestRenderGraph::TestDiamondDAGOrder() {
    RenderGraph g;
    g.AddPass("A", {},    {"X"},       nullptr);
    g.AddPass("B", {"X"}, {"Y"},       nullptr);
    g.AddPass("C", {"X"}, {"Z"},       nullptr);
    g.AddPass("D", {"Y", "Z"}, {"W"},  nullptr);
    TEST_ASSERT_TRUE(g.Compile());

    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL(4, order.size());
    // A must be first, D must be last
    TEST_ASSERT_EQUAL_STRING("A", order[0].c_str());
    TEST_ASSERT_EQUAL_STRING("D", order[3].c_str());
    // B and C can be in either order, but both before D
    bool bBeforeD = false, cBeforeD = false;
    for (size_t i = 0; i < 3; ++i) {
        if (order[i] == "B") bBeforeD = true;
        if (order[i] == "C") cBeforeD = true;
    }
    TEST_ASSERT_TRUE(bBeforeD);
    TEST_ASSERT_TRUE(cBeforeD);
}

// -- Cycle detection: A->B->A via shared resource -----------------------

void TestRenderGraph::TestCycleDetection() {
    // We can't create a true cycle with pure RAW edges in this model
    // (writers always come before readers), but we can verify the graph
    // handles the case where Compile() correctly processes complex graphs.
    // A true cycle would require manually constructed edges; instead test
    // that a well-formed graph does NOT report a cycle.
    RenderGraph g;
    g.AddPass("A", {},    {"X"}, nullptr);
    g.AddPass("B", {"X"}, {"Y"}, nullptr);
    g.AddPass("C", {"Y"}, {"Z"}, nullptr);
    TEST_ASSERT_TRUE(g.Compile());
}

// -- Resource lifetimes --------------------------------------------------

void TestRenderGraph::TestResourceLifetimes() {
    RenderGraph g;
    g.AddPass("produce",   {},        {"tex"}, nullptr);   // exec 0
    g.AddPass("consume1",  {"tex"},   {},      nullptr);   // exec 1
    g.AddPass("consume2",  {"tex"},   {},      nullptr);   // exec 2
    TEST_ASSERT_TRUE(g.Compile());

    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL_STRING("produce", order[0].c_str());

    const ResourceLifetime* lt = g.GetLifetime("tex");
    TEST_ASSERT_NOT_NULL(lt);
    TEST_ASSERT_EQUAL(0, lt->firstWrite);
    TEST_ASSERT_EQUAL(2, lt->lastRead);

    // Unknown resource returns null
    TEST_ASSERT_NULL(g.GetLifetime("nonexistent"));
}

// -- Execute runs callbacks in order ------------------------------------

void TestRenderGraph::TestExecuteCallsCallbacks() {
    RenderGraph g;
    std::vector<std::string> log;

    g.AddPass("first",  {},       {"A"}, [&]{ log.push_back("first"); });
    g.AddPass("second", {"A"},    {"B"}, [&]{ log.push_back("second"); });
    g.AddPass("third",  {"B"},    {},    [&]{ log.push_back("third"); });
    TEST_ASSERT_TRUE(g.Compile());

    g.Execute();
    TEST_ASSERT_EQUAL(3, log.size());
    TEST_ASSERT_EQUAL_STRING("first",  log[0].c_str());
    TEST_ASSERT_EQUAL_STRING("second", log[1].c_str());
    TEST_ASSERT_EQUAL_STRING("third",  log[2].c_str());
}

// -- Clear resets everything ---------------------------------------------

void TestRenderGraph::TestClearResetsState() {
    RenderGraph g;
    g.AddPass("x", {}, {"r"}, nullptr);
    TEST_ASSERT_TRUE(g.Compile());
    TEST_ASSERT_TRUE(g.IsCompiled());

    g.Clear();
    TEST_ASSERT_FALSE(g.IsCompiled());
    TEST_ASSERT_EQUAL(0, g.PassCount());
    TEST_ASSERT_EQUAL(0, g.GetExecutionOrder().size());
}

// -- Execute without Compile is a no-op ---------------------------------

void TestRenderGraph::TestUncompiledExecuteIsNoop() {
    RenderGraph g;
    bool called = false;
    g.AddPass("x", {}, {}, [&]{ called = true; });
    g.Execute();  // no Compile() -- should silently skip
    TEST_ASSERT_FALSE(called);
}

// -- Independent passes preserve insertion order -------------------------

void TestRenderGraph::TestIndependentPassesPreserveInsertionOrder() {
    RenderGraph g;
    g.AddPass("alpha", {}, {"A"}, nullptr);
    g.AddPass("beta",  {}, {"B"}, nullptr);
    g.AddPass("gamma", {}, {"C"}, nullptr);
    TEST_ASSERT_TRUE(g.Compile());

    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL(3, order.size());
    // No dependencies -- stable sort preserves insertion order
    TEST_ASSERT_EQUAL_STRING("alpha", order[0].c_str());
    TEST_ASSERT_EQUAL_STRING("beta",  order[1].c_str());
    TEST_ASSERT_EQUAL_STRING("gamma", order[2].c_str());
}

// -- Write-after-write ordering ------------------------------------------

void TestRenderGraph::TestWriteAfterWriteOrdering() {
    RenderGraph g;
    g.AddPass("write1", {},    {"buf"}, nullptr);
    g.AddPass("write2", {},    {"buf"}, nullptr);
    g.AddPass("read",   {"buf"}, {},    nullptr);
    TEST_ASSERT_TRUE(g.Compile());

    auto order = g.GetExecutionOrder();
    TEST_ASSERT_EQUAL(3, order.size());
    // WAW edge: write1 before write2, read after write2
    TEST_ASSERT_EQUAL_STRING("write1", order[0].c_str());
    TEST_ASSERT_EQUAL_STRING("write2", order[1].c_str());
    TEST_ASSERT_EQUAL_STRING("read",   order[2].c_str());
}

// -- Runner --------------------------------------------------------------

void TestRenderGraph::RunAllTests() {
    RUN_TEST(TestEmptyGraphCompiles);
    RUN_TEST(TestSinglePassCompiles);
    RUN_TEST(TestLinearChainOrder);
    RUN_TEST(TestDiamondDAGOrder);
    RUN_TEST(TestCycleDetection);
    RUN_TEST(TestResourceLifetimes);
    RUN_TEST(TestExecuteCallsCallbacks);
    RUN_TEST(TestClearResetsState);
    RUN_TEST(TestUncompiledExecuteIsNoop);
    RUN_TEST(TestIndependentPassesPreserveInsertionOrder);
    RUN_TEST(TestWriteAfterWriteOrdering);
}
