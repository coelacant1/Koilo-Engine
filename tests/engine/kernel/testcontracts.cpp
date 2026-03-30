// SPDX-License-Identifier: GPL-3.0-or-later
#include "testcontracts.hpp"
#include <koilo/kernel/contracts.hpp>
#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/message_types.hpp>

// ---- Passing contract tests (should not fire) ----

void TestContracts::TestRequirePass() {
    KL_REQUIRE(1 + 1 == 2, "basic arithmetic");
    KL_REQUIRE(true, "literal true");
    int x = 42;
    KL_REQUIRE(x > 0, "positive value");
    TEST_ASSERT_TRUE(true); // reached without abort
}

void TestContracts::TestEnsurePass() {
    KL_ENSURE(sizeof(int) >= 4, "int is at least 32-bit");
    KL_ENSURE(true, "postcondition holds");
    TEST_ASSERT_TRUE(true);
}

void TestContracts::TestInvariantPass() {
    for (int i = 0; i < 5; ++i) {
        KL_INVARIANT(i >= 0 && i < 5, "loop bounds");
    }
    TEST_ASSERT_TRUE(true);
}

// ---- Utility tests ----

void TestContracts::TestContractKindNames() {
    using namespace koilo;

    TEST_ASSERT_EQUAL_STRING("REQUIRE",   ContractKindName(ContractKind::Require));
    TEST_ASSERT_EQUAL_STRING("ENSURE",    ContractKindName(ContractKind::Ensure));
    TEST_ASSERT_EQUAL_STRING("INVARIANT", ContractKindName(ContractKind::Invariant));
}

void TestContracts::TestContractViolationPayload() {
    using namespace koilo;

    ContractViolation v{};
    v.kind       = ContractKind::Require;
    v.expression = "ptr != nullptr";
    v.message    = "null pointer passed";
    v.file       = "test.cpp";
    v.line       = 42;

    TEST_ASSERT_EQUAL(ContractKind::Require, v.kind);
    TEST_ASSERT_EQUAL_STRING("ptr != nullptr", v.expression);
    TEST_ASSERT_EQUAL_STRING("null pointer passed", v.message);
    TEST_ASSERT_EQUAL_STRING("test.cpp", v.file);
    TEST_ASSERT_EQUAL_INT(42, v.line);

    // Verify it can be wrapped in a Message
    auto msg = MakeMessage(MSG_CONTRACT_VIOLATION, MODULE_KERNEL, v);
    TEST_ASSERT_EQUAL(MSG_CONTRACT_VIOLATION, msg.type);
    TEST_ASSERT_EQUAL(sizeof(ContractViolation), msg.size);
    TEST_ASSERT_NOT_NULL(msg.payload);

    auto* p = static_cast<const ContractViolation*>(msg.payload);
    TEST_ASSERT_EQUAL(ContractKind::Require, p->kind);
    TEST_ASSERT_EQUAL_STRING("ptr != nullptr", p->expression);
}

void TestContracts::TestMessageBusGlobalAccessor() {
    using namespace koilo;

    // Before any kernel, global should be null
    MessageBus* prev = GetMessageBus();

    // Set a local bus as global
    MessageBus localBus(64);
    SetMessageBus(&localBus);
    TEST_ASSERT_EQUAL_PTR(&localBus, GetMessageBus());

    // Clear it
    SetMessageBus(nullptr);
    TEST_ASSERT_NULL(GetMessageBus());

    // Restore previous state
    SetMessageBus(prev);
}

// ---- Runner ----

void TestContracts::RunAllTests() {
    RUN_TEST(TestRequirePass);
    RUN_TEST(TestEnsurePass);
    RUN_TEST(TestInvariantPass);
    RUN_TEST(TestContractKindNames);
    RUN_TEST(TestContractViolationPayload);
    RUN_TEST(TestMessageBusGlobalAccessor);
}
