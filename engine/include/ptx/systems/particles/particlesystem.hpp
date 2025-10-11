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
#include "../../registry/reflect_macros.hpp"

namespace ptx {

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
     * @param deltaTime Time since last update (seconds).
     */
    void Update(float deltaTime);

    // === Statistics ===

    /**
     * @brief Gets the total number of active particles across all emitters.
     */
    int GetTotalActiveParticles() const;

    PTX_BEGIN_FIELDS(ParticleSystem)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ParticleSystem)
        PTX_METHOD_AUTO(ParticleSystem, ClearEmitters, "Clear emitters"),
        PTX_METHOD_AUTO(ParticleSystem, GetEmitterCount, "Get emitter count"),
        PTX_METHOD_AUTO(ParticleSystem, Update, "Update"),
        PTX_METHOD_AUTO(ParticleSystem, GetTotalActiveParticles, "Get total active particles")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ParticleSystem)
        PTX_CTOR0(ParticleSystem)
    PTX_END_DESCRIBE(ParticleSystem)
};

} // namespace ptx
