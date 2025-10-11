#include <ptx/systems/particles/particlesystem.hpp>
#include <algorithm>

namespace ptx {

ParticleSystem::ParticleSystem() {
}

ParticleSystem::~ParticleSystem() {
    ClearEmitters();
}

std::shared_ptr<ParticleEmitter> ParticleSystem::CreateEmitter() {
    auto emitter = std::make_shared<ParticleEmitter>();
    emitters.push_back(emitter);
    return emitter;
}

std::shared_ptr<ParticleEmitter> ParticleSystem::CreateEmitter(const ParticleEmitterConfig& config) {
    auto emitter = std::make_shared<ParticleEmitter>(config);
    emitters.push_back(emitter);
    return emitter;
}

void ParticleSystem::RemoveEmitter(std::shared_ptr<ParticleEmitter> emitter) {
    emitters.erase(
        std::remove(emitters.begin(), emitters.end(), emitter),
        emitters.end()
    );
}

void ParticleSystem::ClearEmitters() {
    emitters.clear();
}

void ParticleSystem::Update(float deltaTime) {
    for (auto& emitter : emitters) {
        emitter->Update(deltaTime);
    }
}

int ParticleSystem::GetTotalActiveParticles() const {
    int total = 0;
    for (const auto& emitter : emitters) {
        total += emitter->GetActiveParticleCount();
    }
    return total;
}

} // namespace ptx
