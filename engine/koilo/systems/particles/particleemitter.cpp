// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/particles/particleemitter.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <cmath>
#include <cstdlib>

namespace koilo {

koilo::ParticleEmitter::ParticleEmitter()
    : transform(), config(), emissionTimer(0.0f), durationTimer(0.0f), isPlaying(false), renderSize_(1) {
    particles.resize(config.maxParticles);
}

koilo::ParticleEmitter::ParticleEmitter(const ParticleEmitterConfig& cfg)
    : transform(), config(cfg), emissionTimer(0.0f), durationTimer(0.0f), isPlaying(false), renderSize_(1) {
    particles.resize(config.maxParticles);
}

koilo::ParticleEmitter::~ParticleEmitter() {
}

// === Playback Control ===

void koilo::ParticleEmitter::Play() {
    isPlaying = true;
    durationTimer = 0.0f;
}

void koilo::ParticleEmitter::Stop() {
    isPlaying = false;
    Clear();
}

void koilo::ParticleEmitter::Pause() {
    isPlaying = false;
}

// === Update ===

void koilo::ParticleEmitter::Update() {
    float deltaTime = TimeManager::GetInstance().GetDeltaTime();
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

        particle.color.X = particle.colorStart.X + (particle.colorEnd.X - particle.colorStart.X) * t;
        particle.color.Y = particle.colorStart.Y + (particle.colorEnd.Y - particle.colorStart.Y) * t;
        particle.color.Z = particle.colorStart.Z + (particle.colorEnd.Z - particle.colorStart.Z) * t;

        particle.alpha = particle.alphaStart + (particle.alphaEnd - particle.alphaStart) * t;

        // Apply custom update callbacks
        for (auto& callback : updateCallbacks) {
            callback(particle, deltaTime);
        }
    }
}

// === Configuration ===

void koilo::ParticleEmitter::SetConfig(const ParticleEmitterConfig& cfg) {
    config = cfg;

    // Resize particle pool if necessary
    if (particles.size() != static_cast<size_t>(config.maxParticles)) {
        particles.resize(config.maxParticles);
    }
}

int koilo::ParticleEmitter::GetActiveParticleCount() const {
    int count = 0;
    for (const Particle& particle : particles) {
        if (particle.active) {
            count++;
        }
    }
    return count;
}

void koilo::ParticleEmitter::Clear() {
    for (Particle& particle : particles) {
        particle.active = false;
    }
    emissionTimer = 0.0f;
    durationTimer = 0.0f;
}

// === Custom Updates ===

void koilo::ParticleEmitter::AddUpdateCallback(ParticleUpdateCallback callback) {
    updateCallbacks.push_back(callback);
}

void koilo::ParticleEmitter::ClearUpdateCallbacks() {
    updateCallbacks.clear();
}

// === Emission ===

void koilo::ParticleEmitter::Emit() {
    Particle* particle = GetInactiveParticle();
    if (particle == nullptr) {
        return;  // No free particles
    }

    InitializeParticle(*particle);
}

void koilo::ParticleEmitter::EmitBurst(int count) {
    for (int i = 0; i < count; ++i) {
        Emit();
    }
}

// === Private Methods ===

void koilo::ParticleEmitter::InitializeParticle(Particle& particle) {
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
        float radius = config.shapeSize.X;

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
        float angle = config.shapeSize.X;  // Cone angle
        float randomAngle = RandomRange(-angle, angle);
        float randomRotation = RandomRange(0.0f, 2.0f * 3.14159f);

        Vector3D direction(
            std::sin(randomAngle) * std::cos(randomRotation),
            std::cos(randomAngle),
            std::sin(randomAngle) * std::sin(randomRotation)
        );

        particle.position = emitterPos;
        particle.velocity = direction * RandomRange(config.velocityMin.Magnitude(), config.velocityMax.Magnitude());
        break;
    }

    case EmitterShape::Circle: {
        // Random point on circle
        float angle = RandomRange(0.0f, 2.0f * 3.14159f);
        float radius = config.shapeSize.X;

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

float koilo::ParticleEmitter::RandomRange(float min, float max) {
    float random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return min + random * (max - min);
}

Vector3D koilo::ParticleEmitter::RandomRange(const Vector3D& min, const Vector3D& max) {
    return Vector3D(
        RandomRange(min.X, max.X),
        RandomRange(min.Y, max.Y),
        RandomRange(min.Z, max.Z)
    );
}

Particle* koilo::ParticleEmitter::GetInactiveParticle() {
    for (Particle& particle : particles) {
        if (!particle.active) {
            return &particle;
        }
    }
    return nullptr;
}

// === Rendering ===

void koilo::ParticleEmitter::Render(Color888* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) return;

    for (const Particle& p : particles) {
        if (!p.active) continue;

        uint8_t cr = static_cast<uint8_t>(Mathematics::Constrain(p.color.X * 255.0f, 0.0f, 255.0f));
        uint8_t cg = static_cast<uint8_t>(Mathematics::Constrain(p.color.Y * 255.0f, 0.0f, 255.0f));
        uint8_t cb = static_cast<uint8_t>(Mathematics::Constrain(p.color.Z * 255.0f, 0.0f, 255.0f));

        int px = static_cast<int>(p.position.X);
        int py = static_cast<int>(p.position.Y);

        if (renderSize_ <= 1) {
            if (px >= 0 && px < width && py >= 0 && py < height) {
                Color888& dst = buffer[py * width + px];
                // Alpha blend
                float a = p.alpha;
                dst.R = static_cast<uint8_t>(dst.R + (cr - dst.R) * a);
                dst.G = static_cast<uint8_t>(dst.G + (cg - dst.G) * a);
                dst.B = static_cast<uint8_t>(dst.B + (cb - dst.B) * a);
            }
        } else {
            int half = renderSize_ / 2;
            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int sx = px + dx, sy = py + dy;
                    if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                        Color888& dst = buffer[sy * width + sx];
                        float a = p.alpha;
                        dst.R = static_cast<uint8_t>(dst.R + (cr - dst.R) * a);
                        dst.G = static_cast<uint8_t>(dst.G + (cg - dst.G) * a);
                        dst.B = static_cast<uint8_t>(dst.B + (cb - dst.B) * a);
                    }
                }
            }
        }
    }
}

// === Script-friendly setters ===

void koilo::ParticleEmitter::SetPosition(float x, float y, float z) {
    transform.SetPosition(Vector3D(x, y, z));
}

void koilo::ParticleEmitter::SetEmissionRate(float rate) {
    config.emissionRate = rate > 0 ? rate : 0;
}

void koilo::ParticleEmitter::SetLifetime(float minL, float maxL) {
    config.lifetimeMin = minL > 0 ? minL : 0.01f;
    config.lifetimeMax = maxL > minL ? maxL : minL;
}

void koilo::ParticleEmitter::SetVelocityRange(float vxMin, float vyMin, float vzMin,
                                              float vxMax, float vyMax, float vzMax) {
    config.velocityMin = Vector3D(vxMin, vyMin, vzMin);
    config.velocityMax = Vector3D(vxMax, vyMax, vzMax);
}

void koilo::ParticleEmitter::SetGravity(float gx, float gy, float gz) {
    config.gravity = Vector3D(gx, gy, gz);
}

void koilo::ParticleEmitter::SetStartColor(float r, float g, float b) {
    config.colorStart = Vector3D(r, g, b);
}

void koilo::ParticleEmitter::SetEndColor(float r, float g, float b) {
    config.colorEnd = Vector3D(r, g, b);
}

void koilo::ParticleEmitter::SetSizeRange(float start, float end) {
    config.sizeStart = start;
    config.sizeEnd = end;
}

void koilo::ParticleEmitter::SetParticleSize(int pixels) {
    renderSize_ = pixels > 0 ? pixels : 1;
}

int koilo::ParticleEmitter::GetParticleSize() const {
    return renderSize_;
}

} // namespace koilo
