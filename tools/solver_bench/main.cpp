// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file main.cpp
 * @brief solver benchmark runner.
 *
 * Standalone executable that runs each registered scene to a fixed
 * simulated duration, recording per-step metrics:
 *   - simulated time
 *   - kinetic energy (J), from PhysicsWorld::ComputeDiagnostics()
 *   - max contact penetration (m), from the most recent debug-contact set
 *   - awake-body count
 *   - wall-clock step duration (microseconds)
 *
 * Notes on metrics:
 *  - Penetration is sampled from `GetDebugContacts()`, which contains the
 *    *pre-final-integrate* manifolds emitted by the last substep's
 *    narrowphase. It is not a perfect post-integrate probe, but it tracks
 *    solver residual quality well enough for relative A/B comparison and
 *    is essentially free.
 *  - Sleep can mask poor solver behavior; scenes with a `-nosleep` suffix
 *    disable per-body sleep so the solver is exercised continuously.
 *  - Solver tag is reported via `--solver <name>`. The current build
 *    only ships PGS+warm-start, so the tag defaults to `pgs`. When TGS
 *    or split-impulse variants land, pass the appropriate flag here so
 *    the CSV stream remains comparable across runs.
 *
 * CLI:
 *   ./koilo_solver_bench [--scene <name>]   filter to one scene
 *                        [--solver <name>]  tag in CSV (default "pgs")
 *                        [--csv per_step|summary|both]   default both
 *                        [--quiet]          suppress non-CSV stderr
 *
 * Output: CSV to stdout. Two table types interleaved by section header:
 *   # per_step
 *   solver,scene,step,t_s,ke_j,max_pen_m,awake,step_us
 *   ...
 *   # summary
 *   solver,scene,steps,sim_s,ke_final,ke_max,pen_max,step_us_avg,step_us_p99,settled_step,explosion
 */
#include "scene.hpp"

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/physicsdiagnostics.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace koilo {
namespace bench {

namespace {

struct StepMetric {
    int   step;
    float tSim;
    float ke;
    float maxPen;
    int   awake;
    long  stepUs;
};

struct RunSummary {
    std::string solver;
    std::string scene;
    int    steps;
    float  simS;
    float  keFinal;
    float  keMax;
    float  penMax;
    double stepUsAvg;
    long   stepUsP99;
    int    settledStep;   // first step where KE drops below 0.5J and stays
    bool   explosion;     // KE ever exceeded 1e4, or any pos became NaN/Inf
};

float MaxPenetration(const PhysicsWorld& w) {
    const auto& contacts = w.GetDebugContacts();
    float m = 0.0f;
    for (const auto& ev : contacts) {
        if (ev.penetration > m) m = ev.penetration;
    }
    return m;
}

int AwakeCount(const PhysicsWorld& w) {
    int c = 0;
    for (int i = 0; i < w.GetBodyCount(); ++i) {
        const RigidBody* b = w.GetBody(i);
        if (b && b->IsDynamic() && !b->IsSleeping()) ++c;
    }
    return c;
}

bool AnyNonFinite(const PhysicsWorld& w) {
    for (int i = 0; i < w.GetBodyCount(); ++i) {
        const RigidBody* b = w.GetBody(i);
        if (!b) continue;
        const auto p = b->GetPose().position;
        const auto v = b->GetVelocity();
        if (!std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z) ||
            !std::isfinite(v.X) || !std::isfinite(v.Y) || !std::isfinite(v.Z)) {
            return true;
        }
    }
    return false;
}

RunSummary RunScene(Scene& scene, const std::string& solverTag,
                    bool emitPerStep,
                    int positionIterationsOverride) {
    PhysicsWorld world;
    scene.Build(world);

    // Tighten the substep budget so Step(dt) executes exactly one fixed substep
    // per call - keeps wall-clock measurement aligned with simulated time and
    // avoids overrun-induced step skipping confusing the metric.
    const float dt = scene.FixedDt();
    world.SetFixedTimestep(dt);
    PhysicsBudget budget;
    budget.maxStepMs   = 1000.0f;  // effectively unbounded for the bench
    budget.maxSubsteps = 1;
    world.SetBudget(budget);

    // Override the solver's position-iteration count for split-impulse A/B.
    // Negative => leave default (currently 3, enabling split-impulse). 0 => off.
    if (positionIterationsOverride >= 0) {
        auto& sc = world.GetSolverConfig();
        sc.positionIterations = positionIterationsOverride;
    }

    // If the scene wants sleep off, also harden the world thresholds - body-
    // level allowSleep is set in scene.Build(), but world wake threshold can
    // still trigger islands sleeping if config is loose.
    if (scene.DisableSleep()) {
        auto& sc = world.GetSleepConfig();
        sc.sleepEnergyThreshold = -1.0f;   // never quiet
    }

    const int totalSteps = static_cast<int>(scene.SimDuration() / dt);

    std::vector<long> stepUs;
    stepUs.reserve(totalSteps);

    RunSummary s{};
    s.solver = solverTag;
    s.scene  = scene.Name();
    s.steps  = totalSteps;
    s.simS   = scene.SimDuration();
    s.settledStep = -1;
    s.explosion   = false;

    int   consecutiveQuiet = 0;
    const int kQuietRequired = static_cast<int>(0.5f / dt);
    const float kSettledKE   = 0.5f;
    const float kExplodeKE   = 1.0e4f;

    for (int step = 0; step < totalSteps; ++step) {
        auto t0 = std::chrono::high_resolution_clock::now();
        world.Step(dt);
        auto t1 = std::chrono::high_resolution_clock::now();
        const long us = static_cast<long>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
        stepUs.push_back(us);

        const auto diag = world.ComputeDiagnostics();
        const float ke   = diag.kineticEnergy;
        const float pen  = MaxPenetration(world);
        const int   awk  = AwakeCount(world);

        if (ke > s.keMax)   s.keMax = ke;
        if (pen > s.penMax) s.penMax = pen;
        if (ke > kExplodeKE || AnyNonFinite(world)) s.explosion = true;

        if (s.settledStep < 0) {
            if (ke < kSettledKE) {
                if (++consecutiveQuiet >= kQuietRequired) {
                    s.settledStep = step - kQuietRequired + 1;
                }
            } else {
                consecutiveQuiet = 0;
            }
        }

        if (emitPerStep) {
            std::printf("%s,%s,%d,%.4f,%.4f,%.6f,%d,%ld\n",
                        s.solver.c_str(), s.scene.c_str(),
                        step, (step + 1) * dt, ke, pen, awk, us);
        }

        s.keFinal = ke;
    }

    // p99 wall-clock.
    std::sort(stepUs.begin(), stepUs.end());
    if (!stepUs.empty()) {
        double sum = 0.0;
        for (long u : stepUs) sum += u;
        s.stepUsAvg = sum / static_cast<double>(stepUs.size());
        const std::size_t p99idx =
            static_cast<std::size_t>(stepUs.size() * 99 / 100);
        s.stepUsP99 = stepUs[std::min(p99idx, stepUs.size() - 1)];
    }
    return s;
}

void PrintSummaryRow(const RunSummary& s) {
    std::printf("%s,%s,%d,%.3f,%.3f,%.3f,%.6f,%.1f,%ld,%d,%d\n",
                s.solver.c_str(), s.scene.c_str(),
                s.steps, s.simS,
                s.keFinal, s.keMax, s.penMax,
                s.stepUsAvg, s.stepUsP99,
                s.settledStep, s.explosion ? 1 : 0);
}

} // namespace
} // namespace bench
} // namespace koilo

int main(int argc, char** argv) {
    using namespace koilo::bench;

    std::string sceneFilter;
    std::string solverTag = "pgs";
    std::string csvMode   = "both";
    bool quiet = false;
    int positerOverride = -1;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", name);
                std::exit(2);
            }
            return argv[++i];
        };
        if      (a == "--scene")   sceneFilter = next("--scene");
        else if (a == "--solver")  solverTag   = next("--solver");
        else if (a == "--csv")     csvMode     = next("--csv");
        else if (a == "--positer") positerOverride = std::stoi(next("--positer"));
        else if (a == "--quiet")   quiet = true;
        else if (a == "--help" || a == "-h") {
            std::puts("usage: koilo_solver_bench [--scene <name>] "
                      "[--solver <tag>] [--csv per_step|summary|both] "
                      "[--positer N] [--quiet]");
            return 0;
        } else {
            std::fprintf(stderr, "unknown arg: %s\n", a.c_str());
            return 2;
        }
    }

    const bool emitPerStep = (csvMode == "per_step" || csvMode == "both");
    const bool emitSummary = (csvMode == "summary"  || csvMode == "both");

    auto scenes = RegisterAll();

    if (emitPerStep) {
        std::puts("# per_step");
        std::puts("solver,scene,step,t_s,ke_j,max_pen_m,awake,step_us");
    }

    std::vector<RunSummary> summaries;
    for (auto& sc : scenes) {
        if (!sceneFilter.empty() && sc->Name() != sceneFilter) continue;
        if (!quiet) {
            std::fprintf(stderr, "[bench] running %s ...\n", sc->Name().c_str());
        }
        summaries.push_back(RunScene(*sc, solverTag, emitPerStep, positerOverride));
    }

    if (emitSummary) {
        std::puts("# summary");
        std::puts("solver,scene,steps,sim_s,ke_final,ke_max,pen_max,"
                  "step_us_avg,step_us_p99,settled_step,explosion");
        for (const auto& s : summaries) PrintSummaryRow(s);
    }

    return 0;
}
