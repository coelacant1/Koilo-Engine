// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/systems/render/graph/frame_composer.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

void RegisterRenderGraphCommands(CommandRegistry& registry) {
    // -- render.passes: list registered pass providers with enabled state --
    registry.Register({
        "render.passes",
        "render.passes",
        "List registered render pass providers",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* composer = kernel.Services().Get<rhi::FrameComposer>("frame_composer");
            if (!composer)
                return ConsoleResult::Error("No frame composer registered");

            auto& providers = composer->GetProviders();
            if (providers.empty())
                return ConsoleResult::Ok("(no pass providers registered)");

            auto phaseName = [](rhi::PassPhase p) -> const char* {
                switch (p) {
                    case rhi::PassPhase::Scene:   return "Scene";
                    case rhi::PassPhase::Post:    return "Post";
                    case rhi::PassPhase::Compose: return "Compose";
                    case rhi::PassPhase::Overlay: return "Overlay";
                    default:                      return "?";
                }
            };

            std::ostringstream os;
            os << std::left << std::setw(20) << "Provider"
               << std::setw(10) << "Phase"
               << "Enabled\n";
            os << std::string(40, '-') << "\n";
            for (auto& p : providers) {
                os << std::left << std::setw(20) << p.name
                   << std::setw(10) << phaseName(p.phase)
                   << (p.enabled ? "yes" : "no") << "\n";
            }
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    // -- render.graph: show the last compiled pass DAG execution order --
    registry.Register({
        "render.graph",
        "render.graph",
        "Show render graph execution order (last frame)",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* composer = kernel.Services().Get<rhi::FrameComposer>("frame_composer");
            if (!composer)
                return ConsoleResult::Error("No frame composer registered");

            auto order = composer->GetExecutionOrder();
            if (order.empty())
                return ConsoleResult::Ok("(graph empty or not yet compiled)");

            std::ostringstream os;
            os << "Execution order (" << order.size() << " passes):\n";
            for (size_t i = 0; i < order.size(); ++i) {
                os << "  " << i << ": " << order[i] << "\n";
            }

            // Show resource lifetimes
            auto& lifetimes = composer->GetLastGraph().GetLifetimes();
            if (!lifetimes.empty()) {
                os << "\nResource lifetimes:\n";
                for (auto& [name, lt] : lifetimes) {
                    os << "  " << std::left << std::setw(20) << name
                       << " write@" << lt.firstWrite
                       << " read@" << lt.lastRead << "\n";
                }
            }
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    // -- render.toggle <provider>: toggle a pass provider on/off --
    registry.Register({
        "render.toggle",
        "render.toggle <provider>",
        "Toggle a render pass provider on/off",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: render.toggle <provider>");

            auto* composer = kernel.Services().Get<rhi::FrameComposer>("frame_composer");
            if (!composer)
                return ConsoleResult::Error("No frame composer registered");

            const auto& name = args[0];
            bool cur = composer->IsProviderEnabled(name);
            composer->SetProviderEnabled(name, !cur);
            return ConsoleResult::Ok(name + ": " + (!cur ? "enabled" : "disabled"));
        },
        // Tab-completer: suggest provider names
        [](KoiloKernel& kernel, const std::vector<std::string>&,
           const std::string& partial) -> std::vector<std::string> {
            auto* composer = kernel.Services().Get<rhi::FrameComposer>("frame_composer");
            if (!composer) return {};
            std::vector<std::string> matches;
            for (auto& p : composer->GetProviders()) {
                if (p.name.compare(0, partial.size(), partial) == 0)
                    matches.push_back(p.name);
            }
            return matches;
        }
    });
}

} // namespace koilo
