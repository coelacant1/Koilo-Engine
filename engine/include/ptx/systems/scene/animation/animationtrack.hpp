/**
 * @file AnimationTrack.h
 * @brief Declares the AnimationTrack template class for managing animation tracks with keyframes.
 *
 * This file contains the definition of the AnimationTrack template class, designed
 * to handle animations with a fixed number of parameters and keyframes.
 * The class integrates functionality for adding parameters, playing, pausing,
 * and updating animation tracks.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include <cstddef>

#include "keyframetrack.hpp"

/**
 * @class AnimationTrack
 * @brief A template class for managing animation tracks with keyframes.
 *
 * This class handles the management of animation parameters and their associated
 * keyframes, supporting playback controls, parameter updates, and interpolation.
 *
 * @tparam maxParameters The maximum number of parameters this animation track can handle.
 * @tparam maxKeyFrames The maximum number of keyframes this animation track can contain.
 */
class AnimationTrack {
protected:
    KeyFrameTrack track;

private:
    /**
     * @brief Pure virtual function to add keyframes to the track.
     *
     * This function must be implemented by derived classes to define
     * how keyframes are added to the animation track.
     */
    virtual void AddKeyFrames() = 0;

public:
    static constexpr std::size_t kDefaultParameterCapacity = KeyFrameTrack::kDefaultParameterCapacity;
    static constexpr std::size_t kDefaultKeyFrameCapacity = KeyFrameTrack::kDefaultKeyFrameCapacity;

    AnimationTrack(float min = 0.0f,
                   float max = 1.0f,
                   KeyFrameInterpolation::InterpolationMethod interpMethod = KeyFrameInterpolation::Cosine,
                   std::size_t parameterCapacity = kDefaultParameterCapacity,
                   std::size_t keyFrameCapacity = kDefaultKeyFrameCapacity);
    virtual ~AnimationTrack() = default;

    /**
     * @brief Starts or resumes playback of the animation track.
     */
    void Play();

    /**
     * @brief Pauses playback of the animation track.
     */
    void Pause();

    /**
     * @brief Restarts the animation track's time to the beginning.
     */
    void RestartTime();

    /**
     * @brief Prints the current time of the animation track.
     *
     * This function outputs the current time of the track for debugging purposes.
     */
    float GetTime() const;

    /**
     * @brief Resets the animation track to its initial state.
     */
    void Reset();

    /**
     * @brief Retrieves the current parameter value of the animation track.
     *
     * @return The current value of the parameter being animated.
     */
    float GetParameterValue() const;

    /**
     * @brief Updates the animation track and returns the current parameter value.
     *
     * This function advances the animation track based on the elapsed time
     * and computes the interpolated parameter value.
     *
     * @return The updated parameter value.
     */
    float Update();

    /**
     * @brief Adds a parameter to the animation track.
     *
     * @param parameter A pointer to the parameter to be animated.
     */
    void AddParameter(float* parameter);

    void SetTime(float time);
    std::size_t GetNumKeyFrames() const;
    std::size_t GetNumParameters() const;
    void AddKeyFrame(float time, float value);
    void AddKeyFrame(const KeyFrame& keyFrame);
    void RemoveKeyFrame(std::size_t index);

    void SetRange(float min, float max);
    void SetMin(float min);
    void SetMax(float max);
    float GetMin() const;
    float GetMax() const;

    void SetPlaybackSpeed(float playbackSpeed);
    float GetPlaybackSpeed() const;

    void SetInterpolationMethod(KeyFrameInterpolation::InterpolationMethod interpMethod);
    KeyFrameInterpolation::InterpolationMethod GetInterpolationMethod() const;

};
