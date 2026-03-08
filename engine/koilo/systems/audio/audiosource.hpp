// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file audiosource.hpp
 * @brief Individual audio source for 3D spatial audio.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <memory>
#include "audioclip.hpp"
#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @enum AudioSourceState
 * @brief Playback state of an audio source.
 */
enum class AudioSourceState : uint8_t {
    Stopped = 0,
    Playing = 1,
    Paused = 2
};

/**
 * @class AudioSource
 * @brief Represents a single audio source in 3D space.
 */
class AudioSource {
private:
    // Audio clip reference
    std::shared_ptr<AudioClip> clip;

    // Playback state
    AudioSourceState state;
    float playbackPosition;  // Current position in seconds

    // 3D spatial properties
    Vector3D position;
    Vector3D velocity;
    float minDistance;  // Distance at which volume is maximum
    float maxDistance;  // Distance at which volume is minimum
    float rolloffFactor;  // How quickly volume decreases with distance

    // Audio properties
    float volume;
    float pitch;
    float pan;  // -1.0 (left) to 1.0 (right)
    bool loop;
    bool spatial;  // If false, plays as 2D sound

    // Priority (0-255, higher = more important)
    int priority;

public:
    /**
     * @brief Default constructor.
     */
    AudioSource();

    /**
     * @brief Constructor with audio clip.
     * @param clip The audio clip to play.
     */
    explicit AudioSource(std::shared_ptr<AudioClip> clip);

    /**
     * @brief Sets the audio clip.
     * @param clip The audio clip to play.
     */
    void SetClip(std::shared_ptr<AudioClip> clip) { this->clip = clip; }

    /**
     * @brief Gets the audio clip.
     */
    std::shared_ptr<AudioClip> GetClip() const { return clip; }

    /**
     * @brief Starts playing the audio.
     */
    void Play();

    /**
     * @brief Pauses the audio.
     */
    void Pause();

    /**
     * @brief Stops the audio and resets playback position.
     */
    void Stop();

    /**
     * @brief Gets the playback state.
     */
    AudioSourceState GetState() const { return state; }

    /**
     * @brief Checks if the audio is currently playing.
     */
    bool IsPlaying() const { return state == AudioSourceState::Playing; }

    /**
     * @brief Checks if the audio is paused.
     */
    bool IsPaused() const { return state == AudioSourceState::Paused; }

    /**
     * @brief Checks if the audio is stopped.
     */
    bool IsStopped() const { return state == AudioSourceState::Stopped; }

    // === Position and Velocity ===

    /**
     * @brief Sets the 3D position of the audio source.
     */
    void SetPosition(const Vector3D& position) { this->position = position; }

    /**
     * @brief Gets the 3D position.
     */
    Vector3D GetPosition() const { return position; }

    /**
     * @brief Sets the velocity for Doppler effect.
     */
    void SetVelocity(const Vector3D& velocity) { this->velocity = velocity; }

    /**
     * @brief Gets the velocity.
     */
    Vector3D GetVelocity() const { return velocity; }

    // === Distance Attenuation ===

    /**
     * @brief Sets the minimum distance (no attenuation below this).
     */
    void SetMinDistance(float minDistance) { this->minDistance = minDistance; }

    /**
     * @brief Gets the minimum distance.
     */
    float GetMinDistance() const { return minDistance; }

    /**
     * @brief Sets the maximum distance (maximum attenuation beyond this).
     */
    void SetMaxDistance(float maxDistance) { this->maxDistance = maxDistance; }

    /**
     * @brief Gets the maximum distance.
     */
    float GetMaxDistance() const { return maxDistance; }

    /**
     * @brief Sets the rolloff factor (how quickly volume decreases).
     */
    void SetRolloffFactor(float rolloff) { this->rolloffFactor = rolloff; }

    /**
     * @brief Gets the rolloff factor.
     */
    float GetRolloffFactor() const { return rolloffFactor; }

    // === Audio Properties ===

    /**
     * @brief Sets the volume (0.0 to 1.0).
     */
    void SetVolume(float volume);

    /**
     * @brief Gets the volume.
     */
    float GetVolume() const { return volume; }

    /**
     * @brief Sets the pitch (0.5 = half speed, 2.0 = double speed).
     */
    void SetPitch(float pitch);

    /**
     * @brief Gets the pitch.
     */
    float GetPitch() const { return pitch; }

    /**
     * @brief Sets the pan (-1.0 = left, 0.0 = center, 1.0 = right).
     */
    void SetPan(float pan);

    /**
     * @brief Gets the pan.
     */
    float GetPan() const { return pan; }

    /**
     * @brief Sets whether the audio should loop.
     */
    void SetLoop(bool loop) { this->loop = loop; }

    /**
     * @brief Checks if the audio is looping.
     */
    bool IsLooping() const { return loop; }

    /**
     * @brief Sets whether the audio is spatial (3D) or not (2D).
     */
    void SetSpatial(bool spatial) { this->spatial = spatial; }

    /**
     * @brief Checks if the audio is spatial.
     */
    bool IsSpatial() const { return spatial; }

    /**
     * @brief Sets the playback priority (0-255).
     */
    void SetPriority(int priority);

    /**
     * @brief Gets the playback priority.
     */
    int GetPriority() const { return priority; }

    // === Playback Control ===

    /**
     * @brief Gets the current playback position in seconds.
     */
    float GetPlaybackPosition() const { return playbackPosition; }

    /**
     * @brief Sets the playback position in seconds.
     */
    void SetPlaybackPosition(float position);

    /**
     * @brief Updates the audio source (called by AudioManager).
     */
    void Update();

    KL_BEGIN_FIELDS(AudioSource)
        KL_FIELD(AudioSource, position, "Position", 0, 0),
        KL_FIELD(AudioSource, velocity, "Velocity", 0, 0),
        KL_FIELD(AudioSource, minDistance, "Min distance", 0.0f, 1000.0f),
        KL_FIELD(AudioSource, maxDistance, "Max distance", 0.0f, 10000.0f),
        KL_FIELD(AudioSource, rolloffFactor, "Rolloff factor", 0.0f, 10.0f),
        KL_FIELD(AudioSource, volume, "Volume", 0.0f, 1.0f),
        KL_FIELD(AudioSource, pitch, "Pitch", 0.1f, 3.0f),
        KL_FIELD(AudioSource, pan, "Pan", -1.0f, 1.0f),
        KL_FIELD(AudioSource, loop, "Loop", 0, 1),
        KL_FIELD(AudioSource, spatial, "Spatial", 0, 1),
        KL_FIELD(AudioSource, priority, "Priority", 0, 255)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AudioSource)
        KL_METHOD_AUTO(AudioSource, Play, "Play"),
        KL_METHOD_AUTO(AudioSource, Pause, "Pause"),
        KL_METHOD_AUTO(AudioSource, Stop, "Stop"),
        KL_METHOD_AUTO(AudioSource, IsPlaying, "Is playing"),
        KL_METHOD_AUTO(AudioSource, IsPaused, "Is paused"),
        KL_METHOD_AUTO(AudioSource, IsStopped, "Is stopped"),
        KL_METHOD_AUTO(AudioSource, SetPosition, "Set position"),
        KL_METHOD_AUTO(AudioSource, GetPosition, "Get position"),
        KL_METHOD_AUTO(AudioSource, SetVolume, "Set volume"),
        KL_METHOD_AUTO(AudioSource, GetVolume, "Get volume"),
        KL_METHOD_AUTO(AudioSource, SetPitch, "Set pitch"),
        KL_METHOD_AUTO(AudioSource, GetPitch, "Get pitch"),
        KL_METHOD_AUTO(AudioSource, SetLoop, "Set loop"),
        KL_METHOD_AUTO(AudioSource, IsLooping, "Is looping")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AudioSource)
        KL_CTOR0(AudioSource),
        KL_CTOR(AudioSource, std::shared_ptr<AudioClip>)
    KL_END_DESCRIBE(AudioSource)
};

} // namespace koilo
