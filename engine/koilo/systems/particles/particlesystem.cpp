// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/particles/particlesystem.hpp>
#include <algorithm>

namespace koilo {

koilo::ParticleSystem::ParticleSystem() {
}

koilo::ParticleSystem::~ParticleSystem() {
    ClearEmitters();
}

std::shared_ptr<ParticleEmitter> koilo::ParticleSystem::CreateEmitter() {
    auto emitter = std::make_shared<ParticleEmitter>();
    emitters.push_back(emitter);
    return emitter;
}

std::shared_ptr<ParticleEmitter> koilo::ParticleSystem::CreateEmitter(const ParticleEmitterConfig& config) {
    auto emitter = std::make_shared<ParticleEmitter>(config);
    emitters.push_back(emitter);
    return emitter;
}

void koilo::ParticleSystem::RemoveEmitter(std::shared_ptr<ParticleEmitter> emitter) {
    emitters.erase(
        std::remove(emitters.begin(), emitters.end(), emitter),
        emitters.end()
    );
}

void koilo::ParticleSystem::ClearEmitters() {
    emitters.clear();
}

void koilo::ParticleSystem::Update() {
    for (auto& emitter : emitters) {
        emitter->Update();
    }
}

int koilo::ParticleSystem::GetTotalActiveParticles() const {
    int total = 0;
    for (const auto& emitter : emitters) {
        total += emitter->GetActiveParticleCount();
    }
    return total;
}

void koilo::ParticleSystem::Render(Color888* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) return;
    for (auto& emitter : emitters) {
        emitter->Render(buffer, width, height);
    }
}

ParticleEmitter* koilo::ParticleSystem::AddEmitter(int maxParticles) {
    ParticleEmitterConfig cfg;
    cfg.maxParticles = maxParticles > 0 ? maxParticles : 64;
    auto emitter = std::make_shared<ParticleEmitter>(cfg);
    emitters.push_back(emitter);
    return emitter.get();
}

ParticleEmitter* koilo::ParticleSystem::GetEmitter(int index) {
    if (index >= 0 && index < static_cast<int>(emitters.size()))
        return emitters[index].get();
    return nullptr;
}

} // namespace koilo
