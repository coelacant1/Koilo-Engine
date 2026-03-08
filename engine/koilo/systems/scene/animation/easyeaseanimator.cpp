// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/animation/easyeaseanimator.hpp>

#include <algorithm>


namespace koilo {

koilo::EasyEaseAnimator::EasyEaseAnimator(std::size_t maxParameters,
                                   InterpolationMethod interpMethod,
                                   float springConstant,
                                   float dampingConstant)
    : capacity_(std::max<std::size_t>(1, maxParameters)),
      defaultMethod_(interpMethod),
      defaultSpringConstant_(springConstant),
      defaultDampingConstant_(dampingConstant),
      dampedSprings_(capacity_),
      rampFilters_(capacity_),
      parameters_(capacity_, nullptr),
      parameterFrame_(capacity_, 0.0f),
      previousSet_(capacity_, 0.0f),
      basis_(capacity_, 0.0f),
      goal_(capacity_, 0.0f),
      interpolationMethods_(capacity_, interpMethod),
      dictionary_(capacity_, 0)
{
    for (auto& spring : dampedSprings_) {
        spring.SetConstants(defaultSpringConstant_, defaultDampingConstant_);
    }
}

void koilo::EasyEaseAnimator::SetConstants(uint16_t dictionaryValue, float springConstant, float damping) {
    const std::size_t index = FindIndex(dictionaryValue);
    if (index != kInvalidIndex) {
        dampedSprings_[index].SetConstants(springConstant, damping);
    }
}

float koilo::EasyEaseAnimator::GetValue(uint16_t dictionaryValue) const {
    const std::size_t index = FindIndex(dictionaryValue);
    if (index != kInvalidIndex && parameters_[index]) {
        return *parameters_[index];
    }
    return 0.0f;
}

float koilo::EasyEaseAnimator::GetTarget(uint16_t dictionaryValue) const {
    const std::size_t index = FindIndex(dictionaryValue);
    if (index != kInvalidIndex) {
        return goal_[index];
    }
    return 0.0f;
}

void koilo::EasyEaseAnimator::AddParameter(float* parameter,
                                    uint16_t dictionaryValue,
                                    uint16_t frames,
                                    float basis,
                                    float goal) {
    if (!parameter || currentParameters_ >= capacity_) {
        return;
    }

    if (FindIndex(dictionaryValue) != kInvalidIndex) {
        return;
    }

    const std::size_t index = currentParameters_++;
    basis_[index] = basis;
    goal_[index] = goal;
    parameters_[index] = parameter;
    parameterFrame_[index] = 0.0f;
    previousSet_[index] = 0.0f;
    dictionary_[index] = dictionaryValue;
    interpolationMethods_[index] = defaultMethod_;

    rampFilters_[index].SetFrames(frames);
    dampedSprings_[index].SetConstants(defaultSpringConstant_, defaultDampingConstant_);
}

void koilo::EasyEaseAnimator::AddParameterByIndex(uint16_t dictionaryValue,
                                                  uint16_t frames,
                                                  float basis,
                                                  float goal) {
    if (currentParameters_ >= capacity_) {
        return;
    }

    if (FindIndex(dictionaryValue) != kInvalidIndex) {
        return;
    }

    // Ensure internal storage has room
    if (internalValues_.size() <= currentParameters_) {
        internalValues_.resize(capacity_, 0.0f);
    }

    const std::size_t index = currentParameters_++;
    internalValues_[index] = basis;
    basis_[index] = basis;
    goal_[index] = goal;
    parameters_[index] = &internalValues_[index];
    parameterFrame_[index] = 0.0f;
    previousSet_[index] = 0.0f;
    dictionary_[index] = dictionaryValue;
    interpolationMethods_[index] = defaultMethod_;

    rampFilters_[index].SetFrames(frames);
    dampedSprings_[index].SetConstants(defaultSpringConstant_, defaultDampingConstant_);
}

void koilo::EasyEaseAnimator::AddParameterFrame(uint16_t dictionaryValue, float value) {
    const std::size_t index = FindIndex(dictionaryValue);
    if (index != kInvalidIndex) {
        parameterFrame_[index] = value;
    }
}

void koilo::EasyEaseAnimator::SetInterpolationMethod(uint16_t dictionaryValue, InterpolationMethod interpMethod) {
    const std::size_t index = FindIndex(dictionaryValue);
    if (index != kInvalidIndex) {
        interpolationMethods_[index] = interpMethod;
    }
}

void koilo::EasyEaseAnimator::Reset() {
    for (std::size_t i = 0; i < currentParameters_; ++i) {
        if (parameters_[i]) {
            *parameters_[i] = basis_[i];
        }
        parameterFrame_[i] = 0.0f;
        previousSet_[i] = 0.0f;
    }
}

void koilo::EasyEaseAnimator::SetParameters() {
    for (std::size_t i = 0; i < currentParameters_; ++i) {
        if (!parameters_[i]) {
            continue;
        }

        const float set = previousSet_[i];
        const float fullRange = Mathematics::Map(set, basis_[i], goal_[i], 0.0f, 1.0f);

        switch (interpolationMethods_[i]) {
            case Cosine:
                *parameters_[i] = Mathematics::CosineInterpolation(basis_[i], goal_[i], fullRange);
                break;
            case Bounce:
                *parameters_[i] = Mathematics::BounceInterpolation(basis_[i], goal_[i], fullRange);
                break;
            case Overshoot:
                *parameters_[i] = dampedSprings_[i].GetCurrentPosition();
                break;
            case Linear:
            default:
                *parameters_[i] = set;
                break;
        }
    }
}

void koilo::EasyEaseAnimator::Update() {
    for (std::size_t i = 0; i < currentParameters_; ++i) {
        const float set = rampFilters_[i].Filter(parameterFrame_[i]);
        const float fullRange = Mathematics::Map(set, basis_[i], goal_[i], 0.0f, 1.0f);

        previousSet_[i] = set;

        if (!parameters_[i]) {
            parameterFrame_[i] = basis_[i];
            continue;
        }

        switch (interpolationMethods_[i]) {
            case Cosine:
                *parameters_[i] = Mathematics::CosineInterpolation(basis_[i], goal_[i], fullRange);
                break;
            case Bounce:
                *parameters_[i] = Mathematics::BounceInterpolation(basis_[i], goal_[i], fullRange);
                break;
            case Overshoot:
                *parameters_[i] = dampedSprings_[i].Calculate(parameterFrame_[i], 0.25f);
                break;
            case Linear:
            default:
                *parameters_[i] = fullRange;
                break;
        }

        parameterFrame_[i] = basis_[i];
    }
}

std::size_t koilo::EasyEaseAnimator::FindIndex(uint16_t dictionaryValue) const {
    for (std::size_t i = 0; i < currentParameters_; ++i) {
        if (dictionary_[i] == dictionaryValue) {
            return i;
        }
    }
    return kInvalidIndex;
}

} // namespace koilo
