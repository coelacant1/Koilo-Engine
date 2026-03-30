// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcontracts.hpp
 * @brief Unit tests for contract macros (KL_REQUIRE, KL_ENSURE, KL_INVARIANT).
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <unity.h>
#include <koilo/kernel/contracts.hpp>
#include <utils/testhelpers.hpp>

class TestContracts {
public:
    static void TestRequirePass();
    static void TestEnsurePass();
    static void TestInvariantPass();
    static void TestContractKindNames();
    static void TestContractViolationPayload();
    static void TestMessageBusGlobalAccessor();

    static void RunAllTests();
};
