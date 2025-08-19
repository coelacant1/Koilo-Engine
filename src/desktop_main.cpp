#include <atomic>
#include <csignal>
#include <thread>
#include <cmath>
#include <cstdio>

#include "../lib/uc3d/core/platform/time.hpp"
#include "minimal_shm/minimalshmproject.hpp"

static MinimalShmProject project;
static std::atomic<bool> g_running{true};
static void handle_sigint(int){ g_running = false; }

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    std::signal(SIGINT, handle_sigint);
    std::signal(SIGTERM, handle_sigint);

    project.Initialize();

    const double target_hz = 60.0;
    const double target_dt_s = 1.0 / target_hz;

    uint32_t t0_ms = uc3d::Time::Millis();
    double time_accum = 0.0;
    const double ratio_period_s = 5.0;

    while (g_running.load(std::memory_order_relaxed)) {
        uint32_t t1_ms = uc3d::Time::Millis();
        float dt_s = float(int32_t(t1_ms - t0_ms)) / 1000.0f;
        t0_ms = t1_ms;

        time_accum += dt_s;
        double ratio = std::fmod(time_accum / ratio_period_s, 1.0);

        project.Animate(static_cast<float>(ratio));
        project.Render();
        project.Display();

        uint32_t t2_ms = uc3d::Time::Millis();
        float frame_time_s = float(int32_t(t2_ms - t1_ms)) / 1000.0f;

        if (frame_time_s < target_dt_s) {
            const double sleep_s = target_dt_s - frame_time_s;
            using namespace std::chrono;
            std::this_thread::sleep_for(duration<double>(sleep_s));
        }
    }
    return 0;
}
