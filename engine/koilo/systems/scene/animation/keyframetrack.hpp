/**
 * @file KeyFrameTrack.h
 * @brief Declares the KeyFrameTrack template class for managing keyframe-based animations.
 *
 * This file defines the KeyFrameTrack class, which provides functionality for managing
 * keyframe-based animations, including parameter updates, interpolation, and playback controls.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

// keyframetrack.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "keyframe.hpp"
#include "../../../core/math/mathematics.hpp"
#include "../../../core/platform/time.hpp"
#include "../../../registry/reflect_macros.hpp"

/**
 * @class KeyFrameInterpolation
 * @brief Base class providing interpolation methods for keyframe animations.
 *
 * This class defines the available interpolation methods, which can be used for
 * smooth transitions between keyframes in animations.
 */
class KeyFrameInterpolation {
public:
    /**
     * @enum InterpolationMethod
     * @brief Enumeration of interpolation methods.
     *
     * Provides various methods for transitioning between keyframe values.
     */
    enum InterpolationMethod {
        Linear, ///< Linear interpolation.
        Cosine, ///< Smooth cosine interpolation.
        Step    ///< Step interpolation (discrete transitions).
    };

    PTX_BEGIN_FIELDS(KeyFrameInterpolation)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(KeyFrameInterpolation)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(KeyFrameInterpolation)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(KeyFrameInterpolation)

};

/**
 * @class KeyFrameTrack
 * @brief A template class for managing animations with multiple parameters and keyframes.
 *
 * The KeyFrameTrack class handles animations by managing a set of parameters and their
 * corresponding keyframes. It supports playback controls, interpolation, and time-based updates.
 *
 * @tparam maxParameters The maximum number of parameters this track can handle.
 * @tparam maxKeyFrames The maximum number of keyframes this track can contain.
 */
class KeyFrameTrack : public KeyFrameInterpolation {
public:
    static constexpr std::size_t kDefaultParameterCapacity = 4;
    static constexpr std::size_t kDefaultKeyFrameCapacity = 16;

    KeyFrameTrack(float min = 0.0f,
                  float max = 1.0f,
                  InterpolationMethod interpMethod = Cosine,
                  std::size_t parameterCapacity = kDefaultParameterCapacity,
                  std::size_t keyFrameCapacity = kDefaultKeyFrameCapacity);

    float GetCurrentTime() const noexcept;
    void SetCurrentTime(float setTime);

    void Pause();
    void Play();

    void SetPlaybackSpeed(float playbackSpeed) noexcept;
    float GetPlaybackSpeed() const noexcept { return playbackSpeed_; }

    void SetInterpolationMethod(InterpolationMethod interpMethod) noexcept { interpMethod_ = interpMethod; }
    InterpolationMethod GetInterpolationMethod() const noexcept { return interpMethod_; }

    void SetMin(float min) noexcept;
    void SetMax(float max) noexcept;
    void SetRange(float min, float max) noexcept;
    float GetMin() const noexcept { return min_; }
    float GetMax() const noexcept { return max_; }

    void AddParameter(float* parameter);

    void AddKeyFrame(float time, float value);
    void AddKeyFrame(const KeyFrame& keyFrame);
    void RemoveKeyFrame(std::size_t index);

    float GetParameterValue() const noexcept;
    void Reset();
    float Update();

    std::size_t GetParameterCapacity() const noexcept { return parameterCapacity_; }
    std::size_t GetKeyFrameCapacity() const noexcept { return keyFrameCapacity_; }
    std::size_t GetParameterCount() const noexcept { return parameters_.size(); }
    std::size_t GetKeyFrameCount() const noexcept { return keyFrames_.size(); }
    bool IsActive() const noexcept { return isActive_; }

private:
    float min_ = 0.0f;
    float max_ = 0.0f;
    float startFrameTime_ = Mathematics::FLTMAX;
    float stopFrameTime_ = Mathematics::FLTMIN;

    float parameterValue_ = 0.0f;
    float currentTime_ = 0.0f;
    float lastUpdateSeconds_ = 0.0f;
    bool isActive_ = true;
    float playbackSpeed_ = 1.0f;

    InterpolationMethod interpMethod_ = Cosine;
    std::size_t parameterCapacity_ = kDefaultParameterCapacity;
    std::size_t keyFrameCapacity_ = kDefaultKeyFrameCapacity;

    std::vector<float*> parameters_;
    std::vector<KeyFrame> keyFrames_;

    float ClampValue(float value) const;
    void InsertKeyFrame(const KeyFrame& keyFrame);
    void UpdateFrameRange();

    PTX_BEGIN_FIELDS(KeyFrameTrack)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(KeyFrameTrack)
        PTX_METHOD_AUTO(KeyFrameTrack, GetCurrentTime, "Get current time"),
        PTX_METHOD_AUTO(KeyFrameTrack, SetCurrentTime, "Set current time"),
        PTX_METHOD_AUTO(KeyFrameTrack, Pause, "Pause"),
        PTX_METHOD_AUTO(KeyFrameTrack, Play, "Play"),
        PTX_METHOD_AUTO(KeyFrameTrack, SetPlaybackSpeed, "Set playback speed"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetPlaybackSpeed, "Get playback speed"),
        PTX_METHOD_AUTO(KeyFrameTrack, SetInterpolationMethod, "Set interpolation method"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetInterpolationMethod, "Get interpolation method"),
        PTX_METHOD_AUTO(KeyFrameTrack, SetMin, "Set min"),
        PTX_METHOD_AUTO(KeyFrameTrack, SetMax, "Set max"),
        PTX_METHOD_AUTO(KeyFrameTrack, SetRange, "Set range"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetMin, "Get min"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetMax, "Get max"),
        PTX_METHOD_AUTO(KeyFrameTrack, AddParameter, "Add parameter"),
        /* Add key frame */ PTX_METHOD_OVLD(KeyFrameTrack, AddKeyFrame, void, float, float),
        /* Add key frame */ PTX_METHOD_OVLD(KeyFrameTrack, AddKeyFrame, void, const KeyFrame &),
        PTX_METHOD_AUTO(KeyFrameTrack, RemoveKeyFrame, "Remove key frame"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetParameterValue, "Get parameter value"),
        PTX_METHOD_AUTO(KeyFrameTrack, Reset, "Reset"),
        PTX_METHOD_AUTO(KeyFrameTrack, Update, "Update"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetParameterCapacity, "Get parameter capacity"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetKeyFrameCapacity, "Get key frame capacity"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetParameterCount, "Get parameter count"),
        PTX_METHOD_AUTO(KeyFrameTrack, GetKeyFrameCount, "Get key frame count"),
        PTX_METHOD_AUTO(KeyFrameTrack, IsActive, "Is active")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(KeyFrameTrack)
        PTX_CTOR(KeyFrameTrack, float, float, InterpolationMethod, std::size_t, std::size_t)
    PTX_END_DESCRIBE(KeyFrameTrack)

};
