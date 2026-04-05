// SPDX-License-Identifier: GPL-3.0-or-later
#include "testmodulefault.hpp"
#include <koilo/kernel/module_fault.hpp>
#include <koilo/kernel/module.hpp>
#include <koilo/kernel/module_manager.hpp>
#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/service_registry.hpp>
#include <koilo/kernel/kernel.hpp>
#include <stdexcept>
#include <cstring>

using namespace koilo;

// --- ModuleFaultRecord unit tests ---

void TestModuleFault::TestFaultRecordBasic() {
    ModuleFaultRecord fr;
    TEST_ASSERT_EQUAL_UINT32(0, fr.consecutiveFaults);
    TEST_ASSERT_EQUAL_UINT32(0, fr.totalFaults);
    TEST_ASSERT_TRUE(fr.lastError.empty());

    fr.RecordFault("test error");
    TEST_ASSERT_EQUAL_UINT32(1, fr.consecutiveFaults);
    TEST_ASSERT_EQUAL_UINT32(1, fr.totalFaults);
    TEST_ASSERT_EQUAL_STRING("test error", fr.lastError.c_str());
    TEST_ASSERT_TRUE(fr.ShouldRestart());
    TEST_ASSERT_FALSE(fr.IsPermanentlyFailed());
}

void TestModuleFault::TestFaultRecordCooldown() {
    ModuleFaultRecord fr;
    fr.RecordFault("err");
    TEST_ASSERT_FALSE(fr.CooldownExpired());

    // Tick partial
    fr.TickCooldown(1.0f);
    TEST_ASSERT_FALSE(fr.CooldownExpired());

    // Tick past cooldown
    fr.TickCooldown(2.0f);
    TEST_ASSERT_TRUE(fr.CooldownExpired());
}

void TestModuleFault::TestFaultRecordPermanentFailure() {
    ModuleFaultRecord fr;
    for (uint32_t i = 0; i < ModuleFaultPolicy::kMaxConsecutiveFaults; ++i) {
        fr.RecordFault("repeated");
    }
    TEST_ASSERT_TRUE(fr.IsPermanentlyFailed());
    TEST_ASSERT_FALSE(fr.ShouldRestart());
    TEST_ASSERT_EQUAL_UINT32(ModuleFaultPolicy::kMaxConsecutiveFaults, fr.consecutiveFaults);
}

void TestModuleFault::TestFaultRecordClearOnSuccess() {
    ModuleFaultRecord fr;
    fr.RecordFault("err1");
    fr.RecordFault("err2");
    TEST_ASSERT_EQUAL_UINT32(2, fr.consecutiveFaults);
    TEST_ASSERT_EQUAL_UINT32(2, fr.totalFaults);

    fr.ClearFault();
    TEST_ASSERT_EQUAL_UINT32(0, fr.consecutiveFaults);
    TEST_ASSERT_EQUAL_UINT32(2, fr.totalFaults);  // Total not reset
}

// --- Integration tests with ModuleManager ---

static int g_tickCount = 0;
static int g_throwAfter = -1;  // Throw on this tick count

static bool StubInit(KoiloKernel&) { return true; }
static void StubTick(float) {
    ++g_tickCount;
    if (g_throwAfter >= 0 && g_tickCount >= g_throwAfter) {
        throw std::runtime_error("module fault test");
    }
}
static void StubShutdown() {}

void TestModuleFault::TestModuleManagerFaultIsolation() {
    g_tickCount = 0;
    g_throwAfter = 1;  // Throw on first tick

    KoiloKernel kernel;

    ModuleDesc desc{};
    desc.name = "test.faulty";
    desc.version = KL_VERSION(1, 0, 0);
    desc.Init = StubInit;
    desc.Tick = StubTick;
    desc.Shutdown = StubShutdown;
    desc.OnMessage = nullptr;

    kernel.Modules().RegisterModule(desc);
    kernel.InitializeModules();

    // Should NOT crash -- fault is caught
    kernel.Tick(0.016f);

    // Module should be faulted, not running
    auto state = kernel.Modules().GetState("test.faulty");
    TEST_ASSERT_TRUE(state == ModuleState::Faulted || state == ModuleState::Error);

    kernel.Shutdown();
    g_throwAfter = -1;
}

void TestModuleFault::TestModuleManagerRestart() {
    g_tickCount = 0;
    g_throwAfter = 1;  // Throw on first tick

    KoiloKernel kernel;

    ModuleDesc desc{};
    desc.name = "test.restartable";
    desc.version = KL_VERSION(1, 0, 0);
    desc.Init = StubInit;
    desc.Tick = StubTick;
    desc.Shutdown = StubShutdown;
    desc.OnMessage = nullptr;

    kernel.Modules().RegisterModule(desc);
    kernel.InitializeModules();

    // Fault it
    kernel.Tick(0.016f);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(ModuleState::Faulted),
                      static_cast<uint8_t>(kernel.Modules().GetState("test.restartable")));

    // Stop throwing, then restart
    g_throwAfter = -1;
    bool restarted = kernel.Modules().RestartModule("test.restartable");
    TEST_ASSERT_TRUE(restarted);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(ModuleState::Running),
                      static_cast<uint8_t>(kernel.Modules().GetState("test.restartable")));

    kernel.Shutdown();
}

void TestModuleFault::TestModuleManagerPermanentDisable() {
    g_tickCount = 0;
    g_throwAfter = 0;  // Throw every tick

    KoiloKernel kernel;

    ModuleDesc desc{};
    desc.name = "test.broken";
    desc.version = KL_VERSION(1, 0, 0);
    desc.Init = StubInit;
    desc.Tick = StubTick;
    desc.Shutdown = StubShutdown;
    desc.OnMessage = nullptr;

    kernel.Modules().RegisterModule(desc);
    kernel.InitializeModules();

    // Each cycle: fault -> cooldown -> restart -> fault again
    // Use large dt to expire cooldown quickly
    for (int i = 0; i < 20; ++i) {
        kernel.Tick(3.0f);  // 3s > 2s cooldown
    }

    auto state = kernel.Modules().GetState("test.broken");
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(ModuleState::Error),
                      static_cast<uint8_t>(state));

    // Cannot restart a permanently failed module
    bool restarted = kernel.Modules().RestartModule("test.broken");
    TEST_ASSERT_FALSE(restarted);

    kernel.Shutdown();
    g_throwAfter = -1;
}

void TestModuleFault::TestModuleStateFaulted() {
    // Verify Faulted is a valid distinct state
    TEST_ASSERT_TRUE(ModuleState::Faulted != ModuleState::Running);
    TEST_ASSERT_TRUE(ModuleState::Faulted != ModuleState::Error);
    TEST_ASSERT_TRUE(static_cast<uint8_t>(ModuleState::Faulted) == 4);
}
