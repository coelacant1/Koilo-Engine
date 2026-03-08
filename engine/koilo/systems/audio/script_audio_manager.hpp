// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_audio_manager.hpp
 * @brief Script-facing audio manager - reflectable wrapper for KoiloScript.
 *
 * Wraps AudioManager for KoiloScript access as the `audio` global.
 */

#pragma once

#include <koilo/systems/audio/audiomanager.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <string>

namespace koilo {

/**
 * @class ScriptAudioManager
 * @brief KoiloScript-facing audio API. Registered as `audio` global.
 */
class ScriptAudioManager {
public:
    ScriptAudioManager();
    ~ScriptAudioManager();

    // Lifecycle
    bool Init(int maxSources);
    void Shutdown();
    void Update();

    // Clip management
    bool LoadClip(const std::string& name, const std::string& filepath);
    void UnloadClip(const std::string& name);
    float GetClipDuration(const std::string& name) const;

    // Playback
    void Play(const std::string& clipName);
    void PlayAtVolume(const std::string& clipName, float volume);
    void PlayLooped(const std::string& clipName, float volume);
    void Play3D(const std::string& clipName, float x, float y, float z);
    void StopAll();
    void PauseAll();
    void ResumeAll();

    // Volume
    void SetMasterVolume(float vol);
    float GetMasterVolume() const;
    void SetSFXVolume(float vol);
    void SetMusicVolume(float vol);

    // Listener
    void SetListenerPosition(float x, float y, float z);

    // Info
    int GetActiveCount() const;
    bool IsInitialized() const;

    // Direct C++ access
    AudioManager* GetManager() { return &manager_; }

    KL_BEGIN_FIELDS(ScriptAudioManager)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ScriptAudioManager)
        KL_METHOD_AUTO(ScriptAudioManager, Init, "Init audio system"),
        KL_METHOD_AUTO(ScriptAudioManager, Shutdown, "Shutdown audio"),
        KL_METHOD_AUTO(ScriptAudioManager, Update, "Update audio"),
        KL_METHOD_AUTO(ScriptAudioManager, LoadClip, "Load audio clip"),
        KL_METHOD_AUTO(ScriptAudioManager, UnloadClip, "Unload clip"),
        KL_METHOD_AUTO(ScriptAudioManager, GetClipDuration, "Get clip duration"),
        KL_METHOD_AUTO(ScriptAudioManager, Play, "Play sound"),
        KL_METHOD_AUTO(ScriptAudioManager, PlayAtVolume, "Play at volume"),
        KL_METHOD_AUTO(ScriptAudioManager, PlayLooped, "Play looped"),
        KL_METHOD_AUTO(ScriptAudioManager, Play3D, "Play 3D sound"),
        KL_METHOD_AUTO(ScriptAudioManager, StopAll, "Stop all sounds"),
        KL_METHOD_AUTO(ScriptAudioManager, PauseAll, "Pause all"),
        KL_METHOD_AUTO(ScriptAudioManager, ResumeAll, "Resume all"),
        KL_METHOD_AUTO(ScriptAudioManager, SetMasterVolume, "Set master volume"),
        KL_METHOD_AUTO(ScriptAudioManager, GetMasterVolume, "Get master volume"),
        KL_METHOD_AUTO(ScriptAudioManager, SetSFXVolume, "Set SFX volume"),
        KL_METHOD_AUTO(ScriptAudioManager, SetMusicVolume, "Set music volume"),
        KL_METHOD_AUTO(ScriptAudioManager, SetListenerPosition, "Set listener position"),
        KL_METHOD_AUTO(ScriptAudioManager, GetActiveCount, "Get active source count"),
        KL_METHOD_AUTO(ScriptAudioManager, IsInitialized, "Is initialized")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ScriptAudioManager)
        KL_CTOR0(ScriptAudioManager)
    KL_END_DESCRIBE(ScriptAudioManager)

private:
    AudioManager manager_;
};

} // namespace koilo
