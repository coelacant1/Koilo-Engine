// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testelfloader.hpp
 * @brief Unit tests for the KSL ELF loader.
 */

#pragma once

#include <unity.h>
#include <koilo/ksl/ksl_elf_loader.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <utils/testhelpers.hpp>

class TestElfLoader {
public:
    // ELF validation tests
    static void TestRejectEmptyData();
    static void TestRejectBadMagic();
    static void TestRejectBigEndian();
    static void TestRejectTruncatedHeader();
    static void TestRejectNonSharedObject();
    static void TestInitialState();
    static void TestMoveConstruct();
    static void TestMoveAssign();
    static void TestUnload();

    // Symbol table tests
    static void TestSymbolTableRegisterAndResolve();
    static void TestSymbolTableResolveUnknown();
    static void TestSymbolTableMathFunctions();

    // Live ELF64 loading (requires build/shaders/*.kso)
    static void TestLoadKSO();
    static void TestFindExport();
    static void TestGetSymbolTyped();
    static void TestCallShadeFn();
    static void TestMultipleLoads();
    static void TestReloadAfterUnload();

    static void RunAllTests();
};
