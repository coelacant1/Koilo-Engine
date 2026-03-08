// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_audio_manager.cpp
 * @brief ScriptAudioManager implementation - bridges AudioManager to KoiloScript.
 */

#include <koilo/systems/audio/script_audio_manager.hpp>

namespace koilo {

ScriptAudioManager::ScriptAudioManager() {}
ScriptAudioManager::~ScriptAudioManager() { manager_.Shutdown(); }

bool ScriptAudioManager::Init(int maxSources) {
    return manager_.Initialize(maxSources);
}

void ScriptAudioManager::Shutdown() {
    manager_.Shutdown();
}

void ScriptAudioManager::Update() {
    manager_.Update();
}

bool ScriptAudioManager::LoadClip(const std::string& name, const std::string& filepath) {
    auto clip = manager_.LoadClip(name, filepath);
    return clip != nullptr;
}

void ScriptAudioManager::UnloadClip(const std::string& name) {
    manager_.UnloadClip(name);
}

float ScriptAudioManager::GetClipDuration(const std::string& name) const {
    auto clip = const_cast<AudioManager&>(manager_).GetClip(name);
    return clip ? clip->GetDuration() : 0.0f;
}

void ScriptAudioManager::Play(const std::string& clipName) {
    manager_.PlaySound(clipName);
}

void ScriptAudioManager::PlayAtVolume(const std::string& clipName, float volume) {
    manager_.PlaySound(clipName, volume);
}

void ScriptAudioManager::PlayLooped(const std::string& clipName, float volume) {
    manager_.PlaySound(clipName, volume, 1.0f, true);
}

void ScriptAudioManager::Play3D(const std::string& clipName, float x, float y, float z) {
    manager_.PlaySound3D(clipName, Vector3D(x, y, z));
}

void ScriptAudioManager::StopAll() {
    manager_.StopAll();
}

void ScriptAudioManager::PauseAll() {
    manager_.PauseAll();
}

void ScriptAudioManager::ResumeAll() {
    manager_.ResumeAll();
}

void ScriptAudioManager::SetMasterVolume(float vol) {
    manager_.SetMasterVolume(vol);
}

float ScriptAudioManager::GetMasterVolume() const {
    return manager_.GetMasterVolume();
}

void ScriptAudioManager::SetSFXVolume(float vol) {
    manager_.SetSFXVolume(vol);
}

void ScriptAudioManager::SetMusicVolume(float vol) {
    manager_.SetMusicVolume(vol);
}

void ScriptAudioManager::SetListenerPosition(float x, float y, float z) {
    manager_.GetListener().SetPosition(Vector3D(x, y, z));
}

int ScriptAudioManager::GetActiveCount() const {
    return manager_.GetActiveSourceCount();
}

bool ScriptAudioManager::IsInitialized() const {
    return const_cast<AudioManager&>(manager_).GetActiveSourceCount() >= 0;
}

} // namespace koilo
