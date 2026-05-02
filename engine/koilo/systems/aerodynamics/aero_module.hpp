// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file aero_module.hpp
 * @brief Aerodynamics system as an IModule.
 *
 * Owns an AerodynamicsWorld and a default ConstantWind (zero by default).
 * In Initialize() it locates the PhysicsModule via kernel services and
 * attaches the aero world's pre-fixed-step callback to its PhysicsWorld.
 *
 * Update(dt) is intentionally a no-op: aero forces are applied per
 * physics substep via the registered callback, NOT per render frame.
 */

#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <memory>

namespace koilo::aero {

class AerodynamicsWorld;
class IWindField;
class ConstantWind;

class AerodynamicsModule : public IModule {
public:
    AerodynamicsModule();
    ~AerodynamicsModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Shutdown() override;

    AerodynamicsWorld* GetWorld() { return world_.get(); }

    /// Returns the built-in ConstantWind owned by this module. Useful for
    /// scripts that just want to set a single global wind without
    /// allocating their own field.
    ConstantWind* GetDefaultWind() { return defaultWind_.get(); }

private:
    std::unique_ptr<AerodynamicsWorld> world_;
    std::unique_ptr<ConstantWind>      defaultWind_;

    KL_BEGIN_FIELDS(AerodynamicsModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AerodynamicsModule)
        KL_METHOD_AUTO(AerodynamicsModule, GetInfo, "Get info"),
        KL_METHOD_AUTO(AerodynamicsModule, Update,  "Update"),
        KL_METHOD_AUTO(AerodynamicsModule, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(AerodynamicsModule, GetWorld, "Get aerodynamics world"),
        KL_METHOD_AUTO(AerodynamicsModule, GetDefaultWind, "Get default ConstantWind")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AerodynamicsModule)
        KL_CTOR0(AerodynamicsModule)
    KL_END_DESCRIBE(AerodynamicsModule)
};

} // namespace koilo::aero
