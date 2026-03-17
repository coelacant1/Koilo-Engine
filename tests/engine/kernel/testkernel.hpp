#pragma once
#include <unity.h>

class TestKernel {
public:
    // Arena allocator
    static void TestArenaAllocateAndReset();
    static void TestArenaAlignment();
    static void TestArenaOverflow();
    static void TestArenaExternalBacking();

    // Pool allocator
    static void TestPoolAllocateAndFree();
    static void TestPoolExhaustion();
    static void TestPoolReset();
    static void TestPoolOwnership();
    static void TestPoolMinBlockSize();

    // Linear allocator
    static void TestLinearAllocateAndReset();
    static void TestLinearMarkers();
    static void TestLinearOverflow();

    // Message bus
    static void TestMessageSubscribeAndDispatch();
    static void TestMessageWildcardSubscriber();
    static void TestMessageUnsubscribe();
    static void TestMessageSendImmediate();
    static void TestMessageRingOverflow();
    static void TestMessageFlush();

    // Capabilities
    static void TestCapBitmaskOperations();
    static void TestCapHasCap();
    static void TestCapNames();

    // Service registry
    static void TestServiceRegisterAndGet();
    static void TestServiceOverwrite();
    static void TestServiceUnregister();
    static void TestServiceList();

    // Module manager
    static void TestModuleRegistration();
    static void TestModuleInitOrder();
    static void TestModuleDependencyResolution();
    static void TestModuleCircularDependency();
    static void TestModuleMissingDependency();
    static void TestModuleCapabilityCheck();

    // Kernel integration
    static void TestKernelLifecycle();
    static void TestKernelFrameCycle();

    static void RunAllTests();
};
