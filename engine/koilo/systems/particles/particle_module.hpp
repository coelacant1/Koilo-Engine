// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file particle_module.hpp
 * @brief Particle system as an IModule with ordered lifecycle.
 *
 * Wraps ParticleSystem in the module lifecycle so it can be
 * initialized, updated, and rendered through the ModuleLoader.
 *
 * @date 01/28/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>

namespace koilo {

class ParticleSystem;

class ParticleModule : public IModule {
public:
    ParticleModule();
    ~ParticleModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    ParticleSystem* GetSystem() { return system_.get(); }

private:
    std::unique_ptr<ParticleSystem> system_;
};

} // namespace koilo
