/**
 * @file RampFilter.h
 * @brief Provides a class for smooth value transitions using a ramp filter.
 *
 * The `RampFilter` class ensures smooth transitions between values by incrementally
 * adjusting towards the target value. It is particularly useful in animations and signal processing.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../math/mathematics.hpp" // Includes mathematical utilities for constraints and operations.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class RampFilter
 * @brief Smooths transitions between values using a linear ramp approach.
 *
 * This class incrementally adjusts an internal filter value towards a target,
 * ensuring smooth transitions.
 */
class RampFilter {
private:
    float increment; ///< The step size for each frame of the transition.
    float filter; ///< The current filtered value.
    float epsilon; ///< A small tolerance to determine when to stop adjusting.

public:
    /**
     * @brief Default constructor for `RampFilter`.
     *
     * Initializes the filter with default parameters.
     */
    RampFilter();

    /**
     * @brief Constructs a `RampFilter` with a specified transition duration and epsilon.
     *
     * @param frames The number of frames for a complete transition.
     * @param epsilon The tolerance for considering the transition complete.
     */
    RampFilter(int frames, float epsilon = 0.01f);

    /**
     * @brief Applies the ramp filter to the specified target value.
     *
     * Smoothly transitions the internal filter value towards the given target.
     *
     * @param value The target value to filter towards.
     * @return The filtered value.
     */
    float Filter(float value);

    /**
     * @brief Sets the increment for each transition step.
     *
     * @param increment The step size for each frame of the transition.
     */
    void SetIncrement(float increment);

    /**
     * @brief Sets the number of frames for a complete transition.
     *
     * Automatically calculates the increment based on the number of frames.
     *
     * @param frames The number of frames for a complete transition.
     */
    void SetFrames(int frames);

    PTX_BEGIN_FIELDS(RampFilter)
        /* TODO: PTX_FIELD(RampFilter, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(RampFilter)
        PTX_METHOD_AUTO(RampFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(RampFilter, SetIncrement, "Set increment"),
        PTX_METHOD_AUTO(RampFilter, SetFrames, "Set frames")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(RampFilter)
        PTX_CTOR0(RampFilter),
        PTX_CTOR(RampFilter, int, float)
    PTX_END_DESCRIBE(RampFilter)
    
};
