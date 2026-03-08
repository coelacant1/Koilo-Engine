// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

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

    KL_BEGIN_FIELDS(ParticleEmitterConfig)
        KL_FIELD(ParticleEmitterConfig, emissionRate, "Emission rate", 0, 0),
        KL_FIELD(ParticleEmitterConfig, maxParticles, "Max particles", 0, 0),
        KL_FIELD(ParticleEmitterConfig, looping, "Looping", 0, 1),
        KL_FIELD(ParticleEmitterConfig, duration, "Duration", 0, 0),
        KL_FIELD(ParticleEmitterConfig, lifetimeMin, "Lifetime min", 0, 0),
        KL_FIELD(ParticleEmitterConfig, lifetimeMax, "Lifetime max", 0, 0),
        KL_FIELD(ParticleEmitterConfig, sizeStart, "Size start", 0, 0),
        KL_FIELD(ParticleEmitterConfig, sizeEnd, "Size end", 0, 0),
        KL_FIELD(ParticleEmitterConfig, colorStart, "Color start", 0, 0),
        KL_FIELD(ParticleEmitterConfig, colorEnd, "Color end", 0, 0),
        KL_FIELD(ParticleEmitterConfig, gravity, "Gravity", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ParticleEmitterConfig)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ParticleEmitterConfig)
        KL_CTOR0(ParticleEmitterConfig)
    KL_END_DESCRIBE(ParticleEmitterConfig)
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
    int renderSize_;                          ///< Pixel size for rendering (1 = single pixel)

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
     */
    void Update();

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

    // === Rendering ===

    /**
     * @brief Render all active particles into a rectangular Color888 buffer.
     * Particle positions are interpreted as screen-space (x,y).
     * @param buffer Row-major Color888 array (width * height).
     * @param width  Buffer width in pixels.
     * @param height Buffer height in pixels.
     */
    void Render(Color888* buffer, int width, int height);

    // === Script-friendly setters ===

    void SetPosition(float x, float y, float z);
    void SetEmissionRate(float rate);
    void SetLifetime(float minL, float maxL);
    void SetVelocityRange(float vxMin, float vyMin, float vzMin, float vxMax, float vyMax, float vzMax);
    void SetGravity(float gx, float gy, float gz);
    void SetStartColor(float r, float g, float b);
    void SetEndColor(float r, float g, float b);
    void SetSizeRange(float start, float end);
    void SetParticleSize(int pixels);
    int GetParticleSize() const;

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

    KL_BEGIN_FIELDS(ParticleEmitter)
        KL_FIELD(ParticleEmitter, transform, "Transform", 0, 0),
        KL_FIELD(ParticleEmitter, config, "Config", 0, 0),
        KL_FIELD(ParticleEmitter, isPlaying, "Is playing", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ParticleEmitter)
        KL_METHOD_AUTO(ParticleEmitter, Play, "Play"),
        KL_METHOD_AUTO(ParticleEmitter, Stop, "Stop"),
        KL_METHOD_AUTO(ParticleEmitter, Pause, "Pause"),
        KL_METHOD_AUTO(ParticleEmitter, IsPlaying, "Is playing"),
        KL_METHOD_AUTO(ParticleEmitter, Update, "Update"),
        KL_METHOD_AUTO(ParticleEmitter, GetActiveParticleCount, "Get active particle count"),
        KL_METHOD_AUTO(ParticleEmitter, Clear, "Clear"),
        KL_METHOD_AUTO(ParticleEmitter, Emit, "Emit"),
        KL_METHOD_AUTO(ParticleEmitter, EmitBurst, "Emit burst"),
        KL_METHOD_AUTO(ParticleEmitter, SetPosition, "Set position"),
        KL_METHOD_AUTO(ParticleEmitter, SetEmissionRate, "Set emission rate"),
        KL_METHOD_AUTO(ParticleEmitter, SetLifetime, "Set lifetime range"),
        KL_METHOD_AUTO(ParticleEmitter, SetVelocityRange, "Set velocity range"),
        KL_METHOD_AUTO(ParticleEmitter, SetGravity, "Set gravity"),
        KL_METHOD_AUTO(ParticleEmitter, SetStartColor, "Set start color"),
        KL_METHOD_AUTO(ParticleEmitter, SetEndColor, "Set end color"),
        KL_METHOD_AUTO(ParticleEmitter, SetSizeRange, "Set size range"),
        KL_METHOD_AUTO(ParticleEmitter, SetParticleSize, "Set render size"),
        KL_METHOD_AUTO(ParticleEmitter, GetParticleSize, "Get render size"),
        KL_METHOD_AUTO(ParticleEmitter, Render, "Render to buffer")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ParticleEmitter)
        KL_CTOR0(ParticleEmitter),
        KL_CTOR(ParticleEmitter, ParticleEmitterConfig)
    KL_END_DESCRIBE(ParticleEmitter)
};

} // namespace koilo
