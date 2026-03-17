// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file gpu_registry.hpp
 * @brief Lightweight GPU device information registry.
 *
 * Stores GPU enumeration data populated by the active display backend
 * (Vulkan or otherwise). Console commands and other subsystems can
 * query GPU info without any graphics API dependency.
 *
 * @date 02/14/2026
 * @author Coela
 */

#include <string>
#include <vector>
#include <cstdint>

namespace koilo {

struct GPUDeviceInfo {
    uint32_t    index      = 0;
    std::string name;
    std::string api;          // "Vulkan", "OpenGL", etc.
    std::string type;         // "discrete", "integrated", "cpu", "virtual"
    uint64_t    vramBytes  = 0;
    bool        isSelected = false;
};

/**
 * @class GPURegistry
 * @brief Singleton registry of available GPU devices.
 *
 * Populated by the display backend during initialization.
 * Queried by console commands (gpu-list, gpu-info) and diagnostics.
 */
class GPURegistry {
public:
    static GPURegistry& Get() {
        static GPURegistry instance;
        return instance;
    }

    void Clear() {
        devices_.clear();
        selectedIndex_ = 0;
        activeAPI_.clear();
    }

    void AddDevice(GPUDeviceInfo info) {
        devices_.push_back(std::move(info));
    }

    void SetSelected(uint32_t index) {
        selectedIndex_ = index;
        for (auto& d : devices_)
            d.isSelected = (d.index == index);
    }

    void SetActiveAPI(const std::string& api) { activeAPI_ = api; }

    const std::vector<GPUDeviceInfo>& GetDevices() const { return devices_; }
    uint32_t GetSelectedIndex() const { return selectedIndex_; }
    const std::string& GetActiveAPI() const { return activeAPI_; }
    bool HasDevices() const { return !devices_.empty(); }

    const GPUDeviceInfo* GetSelected() const {
        for (auto& d : devices_)
            if (d.isSelected) return &d;
        return nullptr;
    }

private:
    GPURegistry() = default;
    std::vector<GPUDeviceInfo> devices_;
    uint32_t selectedIndex_ = 0;
    std::string activeAPI_;
};

} // namespace koilo
