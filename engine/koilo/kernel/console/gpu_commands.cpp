// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/gpu_registry.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

void RegisterGPUCommands(CommandRegistry& registry) {
    // -- gpu.list --
    registry.Register({"gpu.list", "gpu.list", "List available GPU devices",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& reg = GPURegistry::Get();
            if (!reg.HasDevices()) {
                return ConsoleResult::Ok("No GPU devices enumerated (OpenGL backend does not enumerate devices).");
            }

            std::ostringstream os;
            os << reg.GetDevices().size() << " GPU(s) [" << reg.GetActiveAPI() << "]:\n";
            for (auto& d : reg.GetDevices()) {
                os << "  [" << d.index << "] " << d.name
                   << " (" << d.type;
                if (d.vramBytes > 0)
                    os << ", " << (d.vramBytes / (1024 * 1024)) << " MB";
                os << ")";
                if (d.isSelected) os << " *active*";
                os << "\n";
            }
            os << "\nUse --gpu <index> at startup to select a different GPU.";
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });

    // -- gpu.info --
    registry.Register({"gpu.info", "gpu.info", "Show active GPU details",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& reg = GPURegistry::Get();
            auto* gpu = reg.GetSelected();
            if (!gpu) {
                return ConsoleResult::Ok("No GPU selected (software rendering or OpenGL without enumeration).");
            }

            std::ostringstream os;
            os << "Active GPU:\n"
               << "  Name:  " << gpu->name << "\n"
               << "  API:   " << reg.GetActiveAPI() << "\n"
               << "  Type:  " << gpu->type << "\n"
               << "  Index: " << gpu->index << "\n";
            if (gpu->vramBytes > 0)
                os << "  VRAM:  " << (gpu->vramBytes / (1024 * 1024)) << " MB\n";
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });
}

} // namespace koilo
