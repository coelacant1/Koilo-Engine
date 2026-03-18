// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file input_module.hpp
 * @brief Input system as an IModule with ordered lifecycle.
 *
 * Wraps InputManager in the module lifecycle so keyboard, mouse,
 * and gamepad state is managed through the ModuleLoader.
 *
 * @date 01/28/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <koilo/systems/input/inputmanager.hpp>

namespace koilo {

class InputModule : public IModule {
public:
    InputModule();
    ~InputModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Shutdown() override;

    InputManager& GetManager() { return manager_; }

private:
    InputManager manager_;
};

} // namespace koilo
