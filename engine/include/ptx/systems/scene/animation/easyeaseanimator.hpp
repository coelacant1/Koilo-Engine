/**
 * @file EasyEaseAnimator.h
 * @brief Declares the EasyEaseAnimator template class for advanced animation easing.
 *
 * This file defines the EasyEaseAnimator class, which implements the IEasyEaseAnimator
 * interface and provides functionalities for smooth and customizable animations using
 * various interpolation methods, damped springs, and ramp filters.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

#include "ieasyeaseanimator.hpp"

/**
 * @class EasyEaseAnimator
 * @brief Runtime-managed implementation of the IEasyEaseAnimator interface.
 *
 * The animator stores parameter slots in contiguous vectors whose capacity is specified at
 * construction time. Each slot tracks the animated value, interpolation method, ramp filter,
 * and optional damped spring used for overshoot easing.
 */
class EasyEaseAnimator : public IEasyEaseAnimator {
public:
    /**
     * @brief Construct an animator with space for @p maxParameters entries.
     * @param maxParameters Maximum number of animated parameters that can be registered.
     * @param interpMethod Default interpolation method applied to newly added parameters.
     * @param springConstant Default spring constant for overshoot easing.
     * @param dampingConstant Default damping factor for overshoot easing.
     */
    EasyEaseAnimator(std::size_t maxParameters,
                     InterpolationMethod interpMethod = IEasyEaseAnimator::Cosine,
                     float springConstant = 1.0f,
                     float dampingConstant = 0.5f);

    ~EasyEaseAnimator() override = default;

    void SetConstants(uint16_t dictionaryValue, float springConstant, float damping) override;
    float GetValue(uint16_t dictionaryValue) const override;
    float GetTarget(uint16_t dictionaryValue) const override;
    void AddParameter(float* parameter, uint16_t dictionaryValue, uint16_t frames, float basis, float goal) override;
    void AddParameterFrame(uint16_t dictionaryValue, float value) override;
    void SetInterpolationMethod(uint16_t dictionaryValue, InterpolationMethod interpMethod) override;
    void Reset() override;
    void SetParameters() override;
    void Update() override;

    /** @return Maximum number of parameters this animator can manage. */
    std::size_t GetCapacity() const noexcept { return capacity_; }

    /** @return Current number of registered parameters. */
    std::size_t GetParameterCount() const noexcept { return currentParameters_; }

    /** @return Whether the animator is active (reserved for future use). */
    bool IsActive() const noexcept { return isActive_; }

    /** @brief Toggle the active state flag. */
    void SetActive(bool active) noexcept { isActive_ = active; }

private:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    std::size_t FindIndex(uint16_t dictionaryValue) const;

    std::size_t capacity_;
    std::size_t currentParameters_ = 0;
    InterpolationMethod defaultMethod_;
    float defaultSpringConstant_;
    float defaultDampingConstant_;

    std::vector<DampedSpring>       dampedSprings_;
    std::vector<RampFilter>         rampFilters_;
    std::vector<float*>             parameters_;
    std::vector<float>              parameterFrame_;
    std::vector<float>              previousSet_;
    std::vector<float>              basis_;
    std::vector<float>              goal_;
    std::vector<InterpolationMethod> interpolationMethods_;
    std::vector<uint16_t>           dictionary_;
    bool                            isActive_ = true;

    PTX_BEGIN_FIELDS(EasyEaseAnimator)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(EasyEaseAnimator)
        PTX_METHOD_AUTO(EasyEaseAnimator, SetConstants, "Set constants"),
        PTX_METHOD_AUTO(EasyEaseAnimator, GetValue, "Get value"),
        PTX_METHOD_AUTO(EasyEaseAnimator, GetTarget, "Get target"),
        PTX_METHOD_AUTO(EasyEaseAnimator, AddParameter, "Add parameter"),
        PTX_METHOD_AUTO(EasyEaseAnimator, AddParameterFrame, "Add parameter frame"),
        PTX_METHOD_AUTO(EasyEaseAnimator, SetInterpolationMethod, "Set interpolation method"),
        PTX_METHOD_AUTO(EasyEaseAnimator, Reset, "Reset"),
        PTX_METHOD_AUTO(EasyEaseAnimator, SetParameters, "Set parameters"),
        PTX_METHOD_AUTO(EasyEaseAnimator, Update, "Update"),
        PTX_METHOD_AUTO(EasyEaseAnimator, GetCapacity, "Get capacity"),
        PTX_METHOD_AUTO(EasyEaseAnimator, GetParameterCount, "Get parameter count"),
        PTX_METHOD_AUTO(EasyEaseAnimator, IsActive, "Is active"),
        PTX_METHOD_AUTO(EasyEaseAnimator, SetActive, "Set active")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(EasyEaseAnimator)
        PTX_CTOR(EasyEaseAnimator, std::size_t, InterpolationMethod, float, float)
    PTX_END_DESCRIBE(EasyEaseAnimator)

};
