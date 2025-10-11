/**
 * @file particleemitter.hpp
 * @brief Particle emitter for spawning and managing particles.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <functional>
#include "particle.hpp"
#include "../../core/math/transform.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @enum EmitterShape
 * @brief Shape of particle emission.
 */
enum class EmitterShape {
    Point,        ///< Emit from a single point
    Sphere,       ///< Emit from sphere surface
    Box,          ///< Emit from box volume
    Cone,         ///< Emit in cone direction
    Circle        ///< Emit from circle (2D)
};

/**
 * @struct ParticleEmitterConfig
 * @brief Configuration for particle emitter behavior.
 */
struct ParticleEmitterConfig {
    // Emission
    float emissionRate;           ///< Particles per second
    int maxParticles;             ///< Maximum particle count
    bool looping;                 ///< Loop emission
    float duration;               ///< Emission duration (if not looping)

    // Shape
    EmitterShape shape;           ///< Emission shape
    Vector3D shapeSize;           ///< Shape dimensions (radius, box size, cone angle)

    // Particle lifetime
    float lifetimeMin;            ///< Minimum lifetime
    float lifetimeMax;            ///< Maximum lifetime

    // Velocity
    Vector3D velocityMin;         ///< Minimum initial velocity
    Vector3D velocityMax;         ///< Maximum initial velocity

    // Size
    float sizeStart;              ///< Initial size
    float sizeEnd;                ///< Final size

    // Color
    Vector3D colorStart;          ///< Initial color
    Vector3D colorEnd;            ///< Final color

    // Alpha
    float alphaStart;             ///< Initial alpha
    float alphaEnd;               ///< Final alpha

    // Rotation
    float rotationSpeedMin;       ///< Minimum rotation speed
    float rotationSpeedMax;       ///< Maximum rotation speed

    // Physics
    Vector3D gravity;             ///< Gravity acceleration

    /**
     * @brief Default constructor.
     */
    ParticleEmitterConfig()
        : emissionRate(10.0f), maxParticles(100), looping(true), duration(5.0f),
          shape(EmitterShape::Point), shapeSize(1, 1, 1),
          lifetimeMin(1.0f), lifetimeMax(3.0f),
          velocityMin(-1, -1, -1), velocityMax(1, 1, 1),
          sizeStart(1.0f), sizeEnd(0.5f),
          colorStart(1, 1, 1), colorEnd(1, 1, 1),
          alphaStart(1.0f), alphaEnd(0.0f),
          rotationSpeedMin(0.0f), rotationSpeedMax(0.0f),
          gravity(0, -9.8f, 0) {}

    PTX_BEGIN_FIELDS(ParticleEmitterConfig)
        PTX_FIELD(ParticleEmitterConfig, emissionRate, "Emission rate", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, maxParticles, "Max particles", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, looping, "Looping", 0, 1),
        PTX_FIELD(ParticleEmitterConfig, duration, "Duration", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, lifetimeMin, "Lifetime min", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, lifetimeMax, "Lifetime max", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, sizeStart, "Size start", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, sizeEnd, "Size end", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, colorStart, "Color start", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, colorEnd, "Color end", 0, 0),
        PTX_FIELD(ParticleEmitterConfig, gravity, "Gravity", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ParticleEmitterConfig)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ParticleEmitterConfig)
        PTX_CTOR0(ParticleEmitterConfig)
    PTX_END_DESCRIBE(ParticleEmitterConfig)
};

/**
 * @typedef ParticleUpdateCallback
 * @brief Custom particle update function.
 */
using ParticleUpdateCallback = std::function<void(Particle&, float)>;

/**
 * @class ParticleEmitter
 * @brief Emits and manages a pool of particles.
 */
class ParticleEmitter {
private:
    Transform transform;                      ///< Emitter transform
    ParticleEmitterConfig config;             ///< Emitter configuration
    std::vector<Particle> particles;          ///< Particle pool

    float emissionTimer;                      ///< Time since last emission
    float durationTimer;                      ///< Time since emission started
    bool isPlaying;                           ///< Is emitter playing?

    std::vector<ParticleUpdateCallback> updateCallbacks;  ///< Custom update functions

public:
    /**
     * @brief Constructor.
     */
    ParticleEmitter();

    /**
     * @brief Constructor with configuration.
     */
    explicit ParticleEmitter(const ParticleEmitterConfig& cfg);

    /**
     * @brief Destructor.
     */
    ~ParticleEmitter();

    // === Playback Control ===

    /**
     * @brief Starts particle emission.
     */
    void Play();

    /**
     * @brief Stops particle emission.
     */
    void Stop();

    /**
     * @brief Pauses particle emission.
     */
    void Pause();

    /**
     * @brief Checks if emitter is playing.
     */
    bool IsPlaying() const { return isPlaying; }

    // === Update ===

    /**
     * @brief Updates all particles.
     * @param deltaTime Time since last update (seconds).
     */
    void Update(float deltaTime);

    // === Configuration ===

    /**
     * @brief Gets the emitter configuration.
     */
    const ParticleEmitterConfig& GetConfig() const { return config; }

    /**
     * @brief Sets the emitter configuration.
     */
    void SetConfig(const ParticleEmitterConfig& cfg);

    /**
     * @brief Gets the emitter transform.
     */
    Transform& GetTransform() { return transform; }

    /**
     * @brief Gets the emitter transform (const).
     */
    const Transform& GetTransform() const { return transform; }

    // === Particle Access ===

    /**
     * @brief Gets all particles.
     */
    const std::vector<Particle>& GetParticles() const { return particles; }

    /**
     * @brief Gets the number of active particles.
     */
    int GetActiveParticleCount() const;

    /**
     * @brief Clears all particles.
     */
    void Clear();

    // === Custom Updates ===

    /**
     * @brief Adds a custom particle update callback.
     */
    void AddUpdateCallback(ParticleUpdateCallback callback);

    /**
     * @brief Clears all update callbacks.
     */
    void ClearUpdateCallbacks();

    // === Emission ===

    /**
     * @brief Emits a single particle immediately.
     */
    void Emit();

    /**
     * @brief Emits a burst of particles.
     * @param count Number of particles to emit.
     */
    void EmitBurst(int count);

private:
    /**
     * @brief Initializes a particle with random values based on config.
     */
    void InitializeParticle(Particle& particle);

    /**
     * @brief Gets a random value between min and max.
     */
    float RandomRange(float min, float max);

    /**
     * @brief Gets a random vector between min and max.
     */
    Vector3D RandomRange(const Vector3D& min, const Vector3D& max);

    /**
     * @brief Finds an inactive particle slot.
     */
    Particle* GetInactiveParticle();

    PTX_BEGIN_FIELDS(ParticleEmitter)
        PTX_FIELD(ParticleEmitter, transform, "Transform", 0, 0),
        PTX_FIELD(ParticleEmitter, config, "Config", 0, 0),
        PTX_FIELD(ParticleEmitter, isPlaying, "Is playing", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ParticleEmitter)
        PTX_METHOD_AUTO(ParticleEmitter, Play, "Play"),
        PTX_METHOD_AUTO(ParticleEmitter, Stop, "Stop"),
        PTX_METHOD_AUTO(ParticleEmitter, Pause, "Pause"),
        PTX_METHOD_AUTO(ParticleEmitter, IsPlaying, "Is playing"),
        PTX_METHOD_AUTO(ParticleEmitter, Update, "Update"),
        PTX_METHOD_AUTO(ParticleEmitter, GetConfig, "Get config"),
        PTX_METHOD_AUTO(ParticleEmitter, SetConfig, "Set config"),
        PTX_METHOD_AUTO(ParticleEmitter, GetActiveParticleCount, "Get active particle count"),
        PTX_METHOD_AUTO(ParticleEmitter, Clear, "Clear"),
        PTX_METHOD_AUTO(ParticleEmitter, Emit, "Emit"),
        PTX_METHOD_AUTO(ParticleEmitter, EmitBurst, "Emit burst")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ParticleEmitter)
        PTX_CTOR0(ParticleEmitter),
        PTX_CTOR(ParticleEmitter, ParticleEmitterConfig)
    PTX_END_DESCRIBE(ParticleEmitter)
};

} // namespace ptx
