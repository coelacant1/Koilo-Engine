/**
 * @file audiomanager.hpp
 * @brief Central audio management system for sound effects and music.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "audiosource.hpp"
#include "audioclip.hpp"
#include "../../core/mathematics/vector3d.hpp"
#include "../../../registry/reflect_macros.hpp"

namespace ptx {

/**
 * @class AudioListener
 * @brief Represents the audio listener (typically the camera/player).
 */
class AudioListener {
private:
    Vector3D position;
    Vector3D velocity;
    Vector3D forward;
    Vector3D up;

public:
    /**
     * @brief Default constructor.
     */
    AudioListener();

    /**
     * @brief Sets the listener position.
     */
    void SetPosition(const Vector3D& position) { this->position = position; }

    /**
     * @brief Gets the listener position.
     */
    Vector3D GetPosition() const { return position; }

    /**
     * @brief Sets the listener velocity (for Doppler effect).
     */
    void SetVelocity(const Vector3D& velocity) { this->velocity = velocity; }

    /**
     * @brief Gets the listener velocity.
     */
    Vector3D GetVelocity() const { return velocity; }

    /**
     * @brief Sets the listener orientation.
     * @param forward The forward direction.
     * @param up The up direction.
     */
    void SetOrientation(const Vector3D& forward, const Vector3D& up);

    /**
     * @brief Gets the forward direction.
     */
    Vector3D GetForward() const { return forward; }

    /**
     * @brief Gets the up direction.
     */
    Vector3D GetUp() const { return up; }

    PTX_BEGIN_FIELDS(AudioListener)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(AudioListener)
        PTX_METHOD_AUTO(AudioListener, SetPosition, "Set position"),
        PTX_METHOD_AUTO(AudioListener, GetPosition, "Get position"),
        PTX_METHOD_AUTO(AudioListener, SetVelocity, "Set velocity"),
        PTX_METHOD_AUTO(AudioListener, GetVelocity, "Get velocity"),
        PTX_METHOD_AUTO(AudioListener, SetOrientation, "Set orientation"),
        PTX_METHOD_AUTO(AudioListener, GetForward, "Get forward"),
        PTX_METHOD_AUTO(AudioListener, GetUp, "Get up")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(AudioListener)
        PTX_CTOR0(AudioListener)
    PTX_END_DESCRIBE(AudioListener)

};

/**
 * @class AudioManager
 * @brief Central manager for all audio playback and mixing.
 */
class AudioManager {
private:
    // Audio sources
    std::vector<std::shared_ptr<AudioSource>> sources;

    // Audio clips (loaded audio data)
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> clips;

    // Audio listener
    AudioListener listener;

    // Volume controls (0.0 to 1.0)
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    float voiceVolume;

    // Maximum simultaneous sources
    int maxSources;

    // Speed of sound (for Doppler effect)
    float speedOfSound;

    // Doppler factor (0.0 = no Doppler, 1.0 = full Doppler)
    float dopplerFactor;

public:
    /**
     * @brief Default constructor.
     */
    AudioManager();

    /**
     * @brief Destructor.
     */
    ~AudioManager();

    /**
     * @brief Initializes the audio system.
     * @param maxSources Maximum number of simultaneous audio sources.
     * @return True if initialization succeeded.
     */
    bool Initialize(int maxSources = 32);

    /**
     * @brief Shuts down the audio system.
     */
    void Shutdown();

    /**
     * @brief Updates all audio sources. Call once per frame.
     * @param deltaTime Time since last update in seconds.
     */
    void Update(float deltaTime);

    // === Audio Clip Management ===

    /**
     * @brief Loads an audio clip from file.
     * @param name The name to associate with the clip.
     * @param filepath Path to the audio file.
     * @return Shared pointer to the loaded clip, or nullptr on failure.
     */
    std::shared_ptr<AudioClip> LoadClip(const std::string& name, const std::string& filepath);

    /**
     * @brief Gets a loaded audio clip by name.
     * @param name The clip name.
     * @return Shared pointer to the clip, or nullptr if not found.
     */
    std::shared_ptr<AudioClip> GetClip(const std::string& name);

    /**
     * @brief Unloads an audio clip.
     * @param name The clip name.
     */
    void UnloadClip(const std::string& name);

    /**
     * @brief Unloads all audio clips.
     */
    void UnloadAllClips();

    // === Audio Source Management ===

    /**
     * @brief Creates a new audio source.
     * @return Shared pointer to the new source.
     */
    std::shared_ptr<AudioSource> CreateSource();

    /**
     * @brief Creates a new audio source with a clip.
     * @param clip The audio clip to play.
     * @return Shared pointer to the new source.
     */
    std::shared_ptr<AudioSource> CreateSource(std::shared_ptr<AudioClip> clip);

    /**
     * @brief Removes an audio source.
     * @param source The source to remove.
     */
    void RemoveSource(std::shared_ptr<AudioSource> source);

    /**
     * @brief Removes all audio sources.
     */
    void RemoveAllSources();

    /**
     * @brief Gets all active audio sources.
     */
    const std::vector<std::shared_ptr<AudioSource>>& GetSources() const { return sources; }

    // === Quick Play Methods ===

    /**
     * @brief Plays a sound effect (2D, non-spatial).
     * @param clipName Name of the clip to play.
     * @param volume Volume (0.0 to 1.0).
     * @param pitch Pitch multiplier.
     * @param loop Whether to loop.
     * @return Shared pointer to the audio source.
     */
    std::shared_ptr<AudioSource> PlaySound(const std::string& clipName, float volume = 1.0f,
                                           float pitch = 1.0f, bool loop = false);

    /**
     * @brief Plays a sound effect at a 3D position.
     * @param clipName Name of the clip to play.
     * @param position 3D position of the sound.
     * @param volume Volume (0.0 to 1.0).
     * @param pitch Pitch multiplier.
     * @param loop Whether to loop.
     * @return Shared pointer to the audio source.
     */
    std::shared_ptr<AudioSource> PlaySound3D(const std::string& clipName, const Vector3D& position,
                                             float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    /**
     * @brief Stops all audio sources.
     */
    void StopAll();

    /**
     * @brief Pauses all audio sources.
     */
    void PauseAll();

    /**
     * @brief Resumes all paused audio sources.
     */
    void ResumeAll();

    // === Listener Management ===

    /**
     * @brief Gets the audio listener.
     */
    AudioListener& GetListener() { return listener; }

    /**
     * @brief Gets the audio listener (const).
     */
    const AudioListener& GetListener() const { return listener; }

    // === Volume Controls ===

    /**
     * @brief Sets the master volume (0.0 to 1.0).
     */
    void SetMasterVolume(float volume);

    /**
     * @brief Gets the master volume.
     */
    float GetMasterVolume() const { return masterVolume; }

    /**
     * @brief Sets the music volume (0.0 to 1.0).
     */
    void SetMusicVolume(float volume);

    /**
     * @brief Gets the music volume.
     */
    float GetMusicVolume() const { return musicVolume; }

    /**
     * @brief Sets the sound effects volume (0.0 to 1.0).
     */
    void SetSFXVolume(float volume);

    /**
     * @brief Gets the sound effects volume.
     */
    float GetSFXVolume() const { return sfxVolume; }

    /**
     * @brief Sets the voice volume (0.0 to 1.0).
     */
    void SetVoiceVolume(float volume);

    /**
     * @brief Gets the voice volume.
     */
    float GetVoiceVolume() const { return voiceVolume; }

    // === Advanced Settings ===

    /**
     * @brief Sets the speed of sound for Doppler effect.
     * @param speed Speed in units/second (default 343.0 for m/s).
     */
    void SetSpeedOfSound(float speed) { speedOfSound = speed; }

    /**
     * @brief Gets the speed of sound.
     */
    float GetSpeedOfSound() const { return speedOfSound; }

    /**
     * @brief Sets the Doppler factor.
     * @param factor 0.0 = no Doppler, 1.0 = full Doppler.
     */
    void SetDopplerFactor(float factor);

    /**
     * @brief Gets the Doppler factor.
     */
    float GetDopplerFactor() const { return dopplerFactor; }

    /**
     * @brief Gets the maximum number of sources.
     */
    int GetMaxSources() const { return maxSources; }

    /**
     * @brief Gets the number of active sources.
     */
    int GetActiveSourceCount() const;

    PTX_BEGIN_FIELDS(AudioManager)
        PTX_FIELD(AudioManager, masterVolume, "Master volume", 0.0f, 1.0f),
        PTX_FIELD(AudioManager, musicVolume, "Music volume", 0.0f, 1.0f),
        PTX_FIELD(AudioManager, sfxVolume, "SFX volume", 0.0f, 1.0f),
        PTX_FIELD(AudioManager, voiceVolume, "Voice volume", 0.0f, 1.0f),
        PTX_FIELD(AudioManager, maxSources, "Max sources", 1, 128),
        PTX_FIELD(AudioManager, speedOfSound, "Speed of sound", 0.0f, 1000.0f),
        PTX_FIELD(AudioManager, dopplerFactor, "Doppler factor", 0.0f, 2.0f)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(AudioManager)
        PTX_METHOD_AUTO(AudioManager, Initialize, "Initialize"),
        PTX_METHOD_AUTO(AudioManager, Shutdown, "Shutdown"),
        PTX_METHOD_AUTO(AudioManager, Update, "Update"),
        PTX_METHOD_AUTO(AudioManager, LoadClip, "Load clip"),
        PTX_METHOD_AUTO(AudioManager, GetClip, "Get clip"),
        PTX_METHOD_AUTO(AudioManager, UnloadClip, "Unload clip"),
        PTX_METHOD_AUTO(AudioManager, PlaySound, "Play sound"),
        PTX_METHOD_AUTO(AudioManager, PlaySound3D, "Play sound 3D"),
        PTX_METHOD_AUTO(AudioManager, StopAll, "Stop all"),
        PTX_METHOD_AUTO(AudioManager, PauseAll, "Pause all"),
        PTX_METHOD_AUTO(AudioManager, ResumeAll, "Resume all"),
        PTX_METHOD_AUTO(AudioManager, SetMasterVolume, "Set master volume"),
        PTX_METHOD_AUTO(AudioManager, GetMasterVolume, "Get master volume")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(AudioManager)
        PTX_CTOR0(AudioManager)
    PTX_END_DESCRIBE(AudioManager)
};

} // namespace ptx
