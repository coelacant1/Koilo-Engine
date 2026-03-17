// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file physics_module.hpp
 * @brief Physics system as an IModule.
 *
 * Wraps PhysicsWorld (rigid bodies, colliders, collision detection,
 * raycasting) in the module lifecycle.
 */

#pragma once

#include <koilo/kernel/unified_module.hpp>
#include <memory>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class PhysicsWorld;

class PhysicsModule : public IModule {
public:
    PhysicsModule();
    ~PhysicsModule() override;

    ModuleInfo GetInfo() const override;
    bool Initialize(KoiloKernel& kernel) override;
    void Update(float dt) override;
    void Render(Color888* buffer, int width, int height) override;
    void Shutdown() override;

    PhysicsWorld* GetWorld() { return world_.get(); }

private:
    std::unique_ptr<PhysicsWorld> world_;

    KL_BEGIN_FIELDS(PhysicsModule)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(PhysicsModule)
        KL_METHOD_AUTO(PhysicsModule, GetInfo, "Get info"),
        KL_METHOD_AUTO(PhysicsModule, Update, "Update"),
        KL_METHOD_AUTO(PhysicsModule, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(PhysicsModule, GetWorld, "Get world")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsModule)
        KL_CTOR0(PhysicsModule)
    KL_END_DESCRIBE(PhysicsModule)

};

} // namespace koilo
