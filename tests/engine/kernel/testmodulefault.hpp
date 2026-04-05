// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <unity.h>

class TestModuleFault {
public:
    static void RunAllTests() {
        RUN_TEST(TestFaultRecordBasic);
        RUN_TEST(TestFaultRecordCooldown);
        RUN_TEST(TestFaultRecordPermanentFailure);
        RUN_TEST(TestFaultRecordClearOnSuccess);
        RUN_TEST(TestModuleManagerFaultIsolation);
        RUN_TEST(TestModuleManagerRestart);
        RUN_TEST(TestModuleManagerPermanentDisable);
        RUN_TEST(TestModuleStateFaulted);
    }

    static void TestFaultRecordBasic();
    static void TestFaultRecordCooldown();
    static void TestFaultRecordPermanentFailure();
    static void TestFaultRecordClearOnSuccess();
    static void TestModuleManagerFaultIsolation();
    static void TestModuleManagerRestart();
    static void TestModuleManagerPermanentDisable();
    static void TestModuleStateFaulted();
};
