// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file particlesystem.hpp
 * @brief Particle system manager for multiple emitters.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <memory>
#include "particleemitter.hpp"
#include <koilo/core/color/color888.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class ParticleSystem
 * @brief Manages multiple particle emitters.
 */
class ParticleSystem {
private:
    std::vector<std::shared_ptr<ParticleEmitter>> emitters;

public:
    /**
     * @brief Constructor.
     */
    ParticleSystem();

    /**
     * @brief Destructor.
     */
    ~ParticleSystem();

    // === Emitter Management ===

    /**
     * @brief Creates a new particle emitter.
     * @return Pointer to the new emitter.
     */
    std::shared_ptr<ParticleEmitter> CreateEmitter();

    /**
     * @brief Creates a new particle emitter with configuration.
     * @param config Emitter configuration.
     * @return Pointer to the new emitter.
     */
    std::shared_ptr<ParticleEmitter> CreateEmitter(const ParticleEmitterConfig& config);

    /**
     * @brief Removes an emitter.
     * @param emitter Emitter to remove.
     */
    void RemoveEmitter(std::shared_ptr<ParticleEmitter> emitter);

    /**
     * @brief Removes all emitters.
     */
    void ClearEmitters();

    /**
     * @brief Gets all emitters.
     */
    const std::vector<std::shared_ptr<ParticleEmitter>>& GetEmitters() const { return emitters; }

    /**
     * @brief Gets the number of emitters.
     */
    size_t GetEmitterCount() const { return emitters.size(); }

    // === Update ===

    /**
     * @brief Updates all emitters.
     */
    void Update();

    // === Statistics ===

    /**
     * @brief Gets the total number of active particles across all emitters.
     */
    int GetTotalActiveParticles() const;

    /**
     * @brief Render all emitters into a Color888 buffer.
     */
    void Render(Color888* buffer, int width, int height);

    /**
     * @brief Create an emitter and return a raw pointer (script-friendly).
     * @param maxParticles Pool capacity.
     * @return Pointer to created emitter (owned by this system).
     */
    ParticleEmitter* AddEmitter(int maxParticles);

    /**
     * @brief Get an emitter by index (script-friendly).
     */
    ParticleEmitter* GetEmitter(int index);

    KL_BEGIN_FIELDS(ParticleSystem)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ParticleSystem)
        KL_METHOD_AUTO(ParticleSystem, ClearEmitters, "Clear emitters"),
        KL_METHOD_AUTO(ParticleSystem, GetEmitterCount, "Get emitter count"),
        KL_METHOD_AUTO(ParticleSystem, Update, "Update"),
        KL_METHOD_AUTO(ParticleSystem, GetTotalActiveParticles, "Get total active particles"),
        KL_METHOD_AUTO(ParticleSystem, AddEmitter, "Add emitter"),
        KL_METHOD_AUTO(ParticleSystem, GetEmitter, "Get emitter by index"),
        KL_METHOD_AUTO(ParticleSystem, Render, "Render to buffer")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ParticleSystem)
        KL_CTOR0(ParticleSystem)
    KL_END_DESCRIBE(ParticleSystem)
};

} // namespace koilo
