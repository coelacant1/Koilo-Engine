#include "testkernel.hpp"
#include <koilo/kernel/memory/arena_allocator.hpp>
#include <koilo/kernel/memory/pool_allocator.hpp>
#include <koilo/kernel/memory/linear_allocator.hpp>
#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/message_types.hpp>
#include <koilo/kernel/capabilities.hpp>
#include <koilo/kernel/service_registry.hpp>
#include <koilo/kernel/module_manager.hpp>
#include <koilo/kernel/kernel.hpp>
#include <cstring>
#include <vector>
#include <string>

using namespace koilo;

// --- Arena Allocator ---

void TestKernel::TestArenaAllocateAndReset() {
    ArenaAllocator arena(1024);
    TEST_ASSERT_EQUAL(0, arena.Used());
    TEST_ASSERT_EQUAL(1024, arena.Capacity());

    void* p1 = arena.Allocate(64);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT(arena.Used() >= 64);

    void* p2 = arena.Allocate(128);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT(p2 != p1);

    arena.Reset();
    TEST_ASSERT_EQUAL(0, arena.Used());
}

void TestKernel::TestArenaAlignment() {
    ArenaAllocator arena(1024);
    arena.Allocate(1); // offset = 1

    void* p = arena.Allocate(16, 16);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL(0, reinterpret_cast<uintptr_t>(p) % 16);
}

void TestKernel::TestArenaOverflow() {
    ArenaAllocator arena(64);
    void* p1 = arena.Allocate(60);
    TEST_ASSERT_NOT_NULL(p1);

    void* p2 = arena.Allocate(64);
    TEST_ASSERT_NULL(p2);
}

void TestKernel::TestArenaExternalBacking() {
    alignas(16) uint8_t buf[256];
    ArenaAllocator arena(buf, sizeof(buf));

    void* p = arena.Allocate(128);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT(p >= buf && p < buf + sizeof(buf));
}

// --- Pool Allocator ---

void TestKernel::TestPoolAllocateAndFree() {
    PoolAllocator pool(64, 4);
    TEST_ASSERT_EQUAL(4, pool.TotalBlocks());
    TEST_ASSERT_EQUAL(0, pool.UsedBlocks());

    void* p1 = pool.Allocate();
    void* p2 = pool.Allocate();
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_EQUAL(2, pool.UsedBlocks());

    pool.Free(p1);
    TEST_ASSERT_EQUAL(1, pool.UsedBlocks());

    void* p3 = pool.Allocate();
    TEST_ASSERT_NOT_NULL(p3);
    TEST_ASSERT_EQUAL(2, pool.UsedBlocks());
}

void TestKernel::TestPoolExhaustion() {
    PoolAllocator pool(32, 2);
    void* p1 = pool.Allocate();
    void* p2 = pool.Allocate();
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);

    void* p3 = pool.Allocate();
    TEST_ASSERT_NULL(p3);
}

void TestKernel::TestPoolReset() {
    PoolAllocator pool(32, 4);
    pool.Allocate();
    pool.Allocate();
    pool.Allocate();
    TEST_ASSERT_EQUAL(3, pool.UsedBlocks());

    pool.Reset();
    TEST_ASSERT_EQUAL(0, pool.UsedBlocks());
    TEST_ASSERT_EQUAL(4, pool.FreeBlocks());
}

void TestKernel::TestPoolOwnership() {
    PoolAllocator pool(32, 4);
    void* p = pool.Allocate();
    TEST_ASSERT_TRUE(pool.Owns(p));

    int stackVar = 42;
    TEST_ASSERT_FALSE(pool.Owns(&stackVar));
}

void TestKernel::TestPoolMinBlockSize() {
    PoolAllocator pool(1, 4);
    TEST_ASSERT(pool.BlockSize() >= sizeof(void*));

    void* p = pool.Allocate();
    TEST_ASSERT_NOT_NULL(p);
    pool.Free(p);
}

// --- Linear Allocator ---

void TestKernel::TestLinearAllocateAndReset() {
    LinearAllocator lin(512);
    void* p = lin.Allocate(100);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT(lin.Used() >= 100);

    lin.Reset();
    TEST_ASSERT_EQUAL(0, lin.Used());
}

void TestKernel::TestLinearMarkers() {
    LinearAllocator lin(512);
    lin.Allocate(64);
    auto marker = lin.GetMarker();
    size_t savedOffset = marker.offset;

    lin.Allocate(128);
    TEST_ASSERT(lin.Used() > savedOffset);

    lin.ResetToMarker(marker);
    TEST_ASSERT_EQUAL(savedOffset, lin.Used());
}

void TestKernel::TestLinearOverflow() {
    LinearAllocator lin(64);
    void* p1 = lin.Allocate(60);
    TEST_ASSERT_NOT_NULL(p1);

    void* p2 = lin.Allocate(64);
    TEST_ASSERT_NULL(p2);
}

// --- Message Bus ---

void TestKernel::TestMessageSubscribeAndDispatch() {
    MessageBus bus(64);
    int received = 0;
    MessageType lastType = MSG_NONE;

    bus.Subscribe(MSG_USER_BASE, [&](const Message& msg) {
        received++;
        lastType = msg.type;
    });

    bus.Send(MakeSignal(MSG_USER_BASE));
    TEST_ASSERT_EQUAL(0, received);

    bus.Dispatch();
    TEST_ASSERT_EQUAL(1, received);
    TEST_ASSERT_EQUAL(MSG_USER_BASE, lastType);
}

void TestKernel::TestMessageWildcardSubscriber() {
    MessageBus bus(64);
    int received = 0;

    bus.SubscribeAll([&](const Message&) { received++; });

    bus.Send(MakeSignal(MSG_USER_BASE));
    bus.Send(MakeSignal(MSG_USER_BASE + 1));
    bus.Send(MakeSignal(MSG_SHUTDOWN));
    bus.Dispatch();

    TEST_ASSERT_EQUAL(3, received);
}

void TestKernel::TestMessageUnsubscribe() {
    MessageBus bus(64);
    int received = 0;

    auto id = bus.Subscribe(MSG_USER_BASE, [&](const Message&) { received++; });
    bus.Unsubscribe(id);

    bus.Send(MakeSignal(MSG_USER_BASE));
    bus.Dispatch();

    TEST_ASSERT_EQUAL(0, received);
}

void TestKernel::TestMessageSendImmediate() {
    MessageBus bus(64);
    int received = 0;

    bus.Subscribe(MSG_USER_BASE, [&](const Message&) { received++; });
    bus.SendImmediate(MakeSignal(MSG_USER_BASE));

    TEST_ASSERT_EQUAL(1, received);
}

void TestKernel::TestMessageRingOverflow() {
    MessageBus bus(4);
    bus.Subscribe(MSG_USER_BASE, [](const Message&) {});

    for (int i = 0; i < 10; ++i) {
        bus.Send(MakeSignal(MSG_USER_BASE));
    }

    TEST_ASSERT(bus.TotalDropped() > 0);
    TEST_ASSERT_EQUAL(4, bus.PendingCount());
}

void TestKernel::TestMessageFlush() {
    MessageBus bus(64);
    bus.Send(MakeSignal(MSG_USER_BASE));
    bus.Send(MakeSignal(MSG_USER_BASE));
    TEST_ASSERT_EQUAL(2, bus.PendingCount());

    bus.FlushPending();
    TEST_ASSERT_EQUAL(0, bus.PendingCount());
}

// --- Capabilities ---

void TestKernel::TestCapBitmaskOperations() {
    Cap combined = Cap::MemoryAlloc | Cap::FileRead;
    TEST_ASSERT(HasCap(combined, Cap::MemoryAlloc));
    TEST_ASSERT(HasCap(combined, Cap::FileRead));
    TEST_ASSERT_FALSE(HasCap(combined, Cap::Network));

    combined |= Cap::Network;
    TEST_ASSERT(HasCap(combined, Cap::Network));
}

void TestKernel::TestCapHasCap() {
    TEST_ASSERT(HasCap(Cap::All, Cap::Debug));
    TEST_ASSERT(HasCap(Cap::All, Cap::GpuAccess));
    TEST_ASSERT_FALSE(HasCap(Cap::None, Cap::MemoryAlloc));
    TEST_ASSERT(HasCap(CAP_TRUSTED, Cap::ConsoleAdmin));
}

void TestKernel::TestCapNames() {
    TEST_ASSERT_EQUAL_STRING("memory.alloc", CapName(Cap::MemoryAlloc));
    TEST_ASSERT_EQUAL_STRING("debug", CapName(Cap::Debug));
    TEST_ASSERT_EQUAL_STRING("network", CapName(Cap::Network));
}

// --- Service Registry ---

void TestKernel::TestServiceRegisterAndGet() {
    ServiceRegistry reg;
    int myService = 42;
    reg.Register("test.service", &myService);

    auto* result = reg.Get<int>("test.service");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(42, *result);

    auto* missing = reg.Get<int>("nonexistent");
    TEST_ASSERT_NULL(missing);
}

void TestKernel::TestServiceOverwrite() {
    ServiceRegistry reg;
    int a = 1, b = 2;
    reg.Register("svc", &a);
    reg.Register("svc", &b);

    TEST_ASSERT_EQUAL(2, *reg.Get<int>("svc"));
}

void TestKernel::TestServiceUnregister() {
    ServiceRegistry reg;
    int val = 10;
    reg.Register("svc", &val);
    TEST_ASSERT_TRUE(reg.Has("svc"));

    reg.Unregister("svc");
    TEST_ASSERT_FALSE(reg.Has("svc"));
    TEST_ASSERT_NULL(reg.Get<int>("svc"));
}

void TestKernel::TestServiceList() {
    ServiceRegistry reg;
    int a = 1, b = 2;
    reg.Register("alpha", &a);
    reg.Register("beta", &b);

    auto list = reg.List();
    TEST_ASSERT_EQUAL(2, list.size());
}

// --- Module Manager ---

static bool g_moduleAInit = false;
static bool g_moduleBInit = false;
static bool g_moduleAShut = false;
static bool g_moduleBShut = false;
static std::vector<std::string> g_initOrder;

static bool InitModuleA(KoiloKernel&) { g_moduleAInit = true; g_initOrder.push_back("A"); return true; }
static void ShutdownModuleA() { g_moduleAShut = true; }
static bool InitModuleB(KoiloKernel&) { g_moduleBInit = true; g_initOrder.push_back("B"); return true; }
static void ShutdownModuleB() { g_moduleBShut = true; }

void TestKernel::TestModuleRegistration() {
    MessageBus bus;
    ServiceRegistry services;
    ModuleManager mgr(bus, services);

    ModuleDesc desc{};
    desc.name = "test.module";
    desc.version = KL_VERSION(1, 0, 0);

    ModuleId id = mgr.RegisterModule(desc);
    TEST_ASSERT(id != 0);
    TEST_ASSERT_EQUAL(1, mgr.ModuleCount());
}

void TestKernel::TestModuleInitOrder() {
    g_moduleAInit = g_moduleBInit = false;
    g_moduleAShut = g_moduleBShut = false;
    g_initOrder.clear();

    KoiloKernel kernel;

    const char* bDeps[] = {"mod.a"};

    ModuleDesc descA{};
    descA.name = "mod.a";
    descA.Init = InitModuleA;
    descA.Shutdown = ShutdownModuleA;

    ModuleDesc descB{};
    descB.name = "mod.b";
    descB.dependencies = bDeps;
    descB.depCount = 1;
    descB.Init = InitModuleB;
    descB.Shutdown = ShutdownModuleB;

    // Register B first, then A - should still init A before B
    kernel.RegisterModule(descB);
    kernel.RegisterModule(descA);

    bool ok = kernel.InitializeModules();
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(g_moduleAInit);
    TEST_ASSERT_TRUE(g_moduleBInit);
    TEST_ASSERT_EQUAL(2, g_initOrder.size());
    TEST_ASSERT_EQUAL_STRING("A", g_initOrder[0].c_str());
    TEST_ASSERT_EQUAL_STRING("B", g_initOrder[1].c_str());
}

void TestKernel::TestModuleDependencyResolution() {
    g_initOrder.clear();

    KoiloKernel kernel;

    ModuleDesc descA{};
    descA.name = "mod.a";
    descA.Init = [](KoiloKernel&) -> bool { g_initOrder.push_back("A"); return true; };

    ModuleDesc descB{};
    descB.name = "mod.b";
    descB.Init = [](KoiloKernel&) -> bool { g_initOrder.push_back("B"); return true; };

    kernel.RegisterModule(descA);
    kernel.RegisterModule(descB);

    bool ok = kernel.InitializeModules();
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(2, g_initOrder.size());
}

void TestKernel::TestModuleCircularDependency() {
    KoiloKernel kernel;

    const char* aDeps[] = {"mod.b"};
    const char* bDeps[] = {"mod.a"};

    ModuleDesc descA{};
    descA.name = "mod.a";
    descA.dependencies = aDeps;
    descA.depCount = 1;

    ModuleDesc descB{};
    descB.name = "mod.b";
    descB.dependencies = bDeps;
    descB.depCount = 1;

    kernel.RegisterModule(descA);
    kernel.RegisterModule(descB);

    bool ok = kernel.InitializeModules();
    TEST_ASSERT_FALSE(ok);
}

void TestKernel::TestModuleMissingDependency() {
    KoiloKernel kernel;

    const char* deps[] = {"nonexistent.module"};
    ModuleDesc desc{};
    desc.name = "mod.a";
    desc.dependencies = deps;
    desc.depCount = 1;

    kernel.RegisterModule(desc);

    bool ok = kernel.InitializeModules();
    TEST_ASSERT_FALSE(ok);
}

void TestKernel::TestModuleCapabilityCheck() {
    KoiloKernel kernel;

    ModuleDesc desc{};
    desc.name = "restricted.mod";
    desc.requiredCaps = Cap::GpuAccess | Cap::Network;

    // Only grant MemoryAlloc - should fail capability check
    kernel.RegisterModule(desc, Cap::MemoryAlloc);

    bool ok = kernel.InitializeModules();
    TEST_ASSERT_FALSE(ok);
}

// --- Kernel Integration ---

void TestKernel::TestKernelLifecycle() {
    g_moduleAInit = g_moduleAShut = false;

    KoiloKernel kernel;

    ModuleDesc desc{};
    desc.name = "lifecycle.mod";
    desc.Init = [](KoiloKernel&) -> bool { g_moduleAInit = true; return true; };
    desc.Shutdown = []() { g_moduleAShut = true; };

    kernel.RegisterModule(desc);
    TEST_ASSERT_TRUE(kernel.InitializeModules());
    TEST_ASSERT_TRUE(kernel.IsRunning());
    TEST_ASSERT_TRUE(g_moduleAInit);

    kernel.Shutdown();
    TEST_ASSERT_FALSE(kernel.IsRunning());
    TEST_ASSERT_TRUE(g_moduleAShut);
}

void TestKernel::TestKernelFrameCycle() {
    KoiloKernel kernel;
    kernel.InitializeModules();

    void* p = kernel.FrameArena().Allocate(256);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT(kernel.FrameArena().Used() >= 256);

    kernel.BeginFrame();
    kernel.Tick(0.016f);
    kernel.EndFrame();

    // Frame arena should be reset after EndFrame
    TEST_ASSERT_EQUAL(0, kernel.FrameArena().Used());

    kernel.Shutdown();
}

// --- RunAllTests ---

void TestKernel::RunAllTests() {
    // Arena
    RUN_TEST(TestKernel::TestArenaAllocateAndReset);
    RUN_TEST(TestKernel::TestArenaAlignment);
    RUN_TEST(TestKernel::TestArenaOverflow);
    RUN_TEST(TestKernel::TestArenaExternalBacking);

    // Pool
    RUN_TEST(TestKernel::TestPoolAllocateAndFree);
    RUN_TEST(TestKernel::TestPoolExhaustion);
    RUN_TEST(TestKernel::TestPoolReset);
    RUN_TEST(TestKernel::TestPoolOwnership);
    RUN_TEST(TestKernel::TestPoolMinBlockSize);

    // Linear
    RUN_TEST(TestKernel::TestLinearAllocateAndReset);
    RUN_TEST(TestKernel::TestLinearMarkers);
    RUN_TEST(TestKernel::TestLinearOverflow);

    // Message bus
    RUN_TEST(TestKernel::TestMessageSubscribeAndDispatch);
    RUN_TEST(TestKernel::TestMessageWildcardSubscriber);
    RUN_TEST(TestKernel::TestMessageUnsubscribe);
    RUN_TEST(TestKernel::TestMessageSendImmediate);
    RUN_TEST(TestKernel::TestMessageRingOverflow);
    RUN_TEST(TestKernel::TestMessageFlush);

    // Capabilities
    RUN_TEST(TestKernel::TestCapBitmaskOperations);
    RUN_TEST(TestKernel::TestCapHasCap);
    RUN_TEST(TestKernel::TestCapNames);

    // Service registry
    RUN_TEST(TestKernel::TestServiceRegisterAndGet);
    RUN_TEST(TestKernel::TestServiceOverwrite);
    RUN_TEST(TestKernel::TestServiceUnregister);
    RUN_TEST(TestKernel::TestServiceList);

    // Module manager
    RUN_TEST(TestKernel::TestModuleRegistration);
    RUN_TEST(TestKernel::TestModuleInitOrder);
    RUN_TEST(TestKernel::TestModuleDependencyResolution);
    RUN_TEST(TestKernel::TestModuleCircularDependency);
    RUN_TEST(TestKernel::TestModuleMissingDependency);
    RUN_TEST(TestKernel::TestModuleCapabilityCheck);

    // Kernel integration
    RUN_TEST(TestKernel::TestKernelLifecycle);
    RUN_TEST(TestKernel::TestKernelFrameCycle);
}
