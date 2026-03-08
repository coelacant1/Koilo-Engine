#include <ptx/systems/particles/particleemitter.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace ptx {

ParticleEmitter::ParticleEmitter()
    : transform(), config(), emissionTimer(0.0f), durationTimer(0.0f), isPlaying(false) {
    particles.resize(config.maxParticles);
}

ParticleEmitter::ParticleEmitter(const ParticleEmitterConfig& cfg)
    : transform(), config(cfg), emissionTimer(0.0f), durationTimer(0.0f), isPlaying(false) {
    particles.resize(config.maxParticles);
}

ParticleEmitter::~ParticleEmitter() {
}

// === Playback Control ===

void ParticleEmitter::Play() {
    isPlaying = true;
    durationTimer = 0.0f;
}

void ParticleEmitter::Stop() {
    isPlaying = false;
    Clear();
}

void ParticleEmitter::Pause() {
    isPlaying = false;
}

// === Update ===

void ParticleEmitter::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    // Update duration timer
    if (isPlaying) {
        durationTimer += deltaTime;

        // Check if emission should stop (non-looping)
        if (!config.looping && durationTimer >= config.duration) {
            Pause();
        }

        // Emit particles based on emission rate
        if (isPlaying) {
            emissionTimer += deltaTime;
            float emissionInterval = 1.0f / config.emissionRate;

            while (emissionTimer >= emissionInterval) {
                Emit();
                emissionTimer -= emissionInterval;
            }
        }
    }

    // Update all active particles
    for (Particle& particle : particles) {
        if (!particle.active) continue;

        // Update age
        particle.age += deltaTime;

        // Kill old particles
        if (particle.age >= particle.lifetime) {
            particle.active = false;
            continue;
        }

        // Update physics
        particle.velocity = particle.velocity + particle.acceleration * deltaTime;
        particle.position = particle.position + particle.velocity * deltaTime;

        // Update rotation
        particle.rotation += particle.rotationSpeed * deltaTime;

        // Update size, color, alpha based on lifetime progress
        float t = particle.GetLifetimeProgress();

        particle.size = particle.sizeStart + (particle.sizeEnd - particle.sizeStart) * t;

        particle.color.x = particle.colorStart.x + (particle.colorEnd.x - particle.colorStart.x) * t;
        particle.color.y = particle.colorStart.y + (particle.colorEnd.y - particle.colorStart.y) * t;
        particle.color.z = particle.colorStart.z + (particle.colorEnd.z - particle.colorStart.z) * t;

        particle.alpha = particle.alphaStart + (particle.alphaEnd - particle.alphaStart) * t;

        // Apply custom update callbacks
        for (auto& callback : updateCallbacks) {
            callback(particle, deltaTime);
        }
    }
}

// === Configuration ===

void ParticleEmitter::SetConfig(const ParticleEmitterConfig& cfg) {
    config = cfg;

    // Resize particle pool if necessary
    if (particles.size() != static_cast<size_t>(config.maxParticles)) {
        particles.resize(config.maxParticles);
    }
}

int ParticleEmitter::GetActiveParticleCount() const {
    int count = 0;
    for (const Particle& particle : particles) {
        if (particle.active) {
            count++;
        }
    }
    return count;
}

void ParticleEmitter::Clear() {
    for (Particle& particle : particles) {
        particle.active = false;
    }
    emissionTimer = 0.0f;
    durationTimer = 0.0f;
}

// === Custom Updates ===

void ParticleEmitter::AddUpdateCallback(ParticleUpdateCallback callback) {
    updateCallbacks.push_back(callback);
}

void ParticleEmitter::ClearUpdateCallbacks() {
    updateCallbacks.clear();
}

// === Emission ===

void ParticleEmitter::Emit() {
    Particle* particle = GetInactiveParticle();
    if (particle == nullptr) {
        return;  // No free particles
    }

    InitializeParticle(*particle);
}

void ParticleEmitter::EmitBurst(int count) {
    for (int i = 0; i < count; ++i) {
        Emit();
    }
}

// === Private Methods ===

void ParticleEmitter::InitializeParticle(Particle& particle) {
    // Position based on emitter shape
    Vector3D emitterPos = transform.GetPosition();

    switch (config.shape) {
    case EmitterShape::Point:
        particle.position = emitterPos;
        break;

    case EmitterShape::Sphere: {
        // Random point on sphere surface
        float theta = RandomRange(0.0f, 2.0f * 3.14159f);
        float phi = RandomRange(0.0f, 3.14159f);
        float radius = config.shapeSize.x;

        Vector3D offset(
            radius * std::sin(phi) * std::cos(theta),
            radius * std::sin(phi) * std::sin(theta),
            radius * std::cos(phi)
        );

        particle.position = emitterPos + offset;
        break;
    }

    case EmitterShape::Box: {
        // Random point in box volume
        Vector3D offset = RandomRange(-config.shapeSize, config.shapeSize);
        particle.position = emitterPos + offset;
        break;
    }

    case EmitterShape::Cone: {
        // Random direction in cone
        float angle = config.shapeSize.x;  // Cone angle
        float randomAngle = RandomRange(-angle, angle);
        float randomRotation = RandomRange(0.0f, 2.0f * 3.14159f);

        Vector3D direction(
            std::sin(randomAngle) * std::cos(randomRotation),
            std::cos(randomAngle),
            std::sin(randomAngle) * std::sin(randomRotation)
        );

        particle.position = emitterPos;
        particle.velocity = direction * RandomRange(config.velocityMin.length(), config.velocityMax.length());
        break;
    }

    case EmitterShape::Circle: {
        // Random point on circle
        float angle = RandomRange(0.0f, 2.0f * 3.14159f);
        float radius = config.shapeSize.x;

        Vector3D offset(
            radius * std::cos(angle),
            0.0f,
            radius * std::sin(angle)
        );

        particle.position = emitterPos + offset;
        break;
    }
    }

    // Initialize particle properties
    particle.velocity = RandomRange(config.velocityMin, config.velocityMax);
    particle.acceleration = config.gravity;

    particle.lifetime = RandomRange(config.lifetimeMin, config.lifetimeMax);
    particle.age = 0.0f;

    particle.sizeStart = config.sizeStart;
    particle.sizeEnd = config.sizeEnd;
    particle.size = particle.sizeStart;

    particle.colorStart = config.colorStart;
    particle.colorEnd = config.colorEnd;
    particle.color = particle.colorStart;

    particle.alphaStart = config.alphaStart;
    particle.alphaEnd = config.alphaEnd;
    particle.alpha = particle.alphaStart;

    particle.rotation = 0.0f;
    particle.rotationSpeed = RandomRange(config.rotationSpeedMin, config.rotationSpeedMax);

    particle.active = true;
}

float ParticleEmitter::RandomRange(float min, float max) {
    float random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return min + random * (max - min);
}

Vector3D ParticleEmitter::RandomRange(const Vector3D& min, const Vector3D& max) {
    return Vector3D(
        RandomRange(min.x, max.x),
        RandomRange(min.y, max.y),
        RandomRange(min.z, max.z)
    );
}

Particle* ParticleEmitter::GetInactiveParticle() {
    for (Particle& particle : particles) {
        if (!particle.active) {
            return &particle;
        }
    }
    return nullptr;
}

} // namespace ptx
