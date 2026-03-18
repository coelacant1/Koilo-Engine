// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/register_services.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/debug/profiler.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <koilo/systems/profiling/memoryprofiler.hpp>

namespace koilo {

void RegisterCoreServices(KoiloKernel& kernel) {
    auto& svc = kernel.Services();
    // These retrieve the kernel-owned instances (installed via SetInstance)
    svc.Register("time",         &TimeManager::GetInstance());
    svc.Register("debug_draw",   &DebugDraw::GetInstance());
    svc.Register("profiler",     &Profiler::GetInstance());
    svc.Register("perf_profiler",&PerformanceProfiler::GetInstance());
    svc.Register("mem_profiler", &MemoryProfiler::GetInstance());
    svc.Register("sky",          &Sky::GetInstance());
}

} // namespace koilo
