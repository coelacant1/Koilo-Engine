#pragma once

#include "../effect.hpp"
#include "../../core/ipixelgroup.hpp"
#include "../../../../core/signal/functiongenerator.hpp"
#include "../../../../core/math/vector2d.hpp"
#include <cmath>
#include "../../../../registry/reflect_macros.hpp"

/**
 * @class Fisheye
 * @brief Animated fisheye warp over the camera's pixel buffer.
 *
 * Uses ratio (0..1) from Effect::SetRatio to scale the animated displacement.
 * Writes result into the group's temporary color buffer, then copies back.
 */
class Fisheye : public Effect {
private:
    Vector2D         offset {0.0f, 0.0f};   // center offset for fisheye
    float            amplitude = 0.5f;      // warp exponent
    // Animated generators (center, radius, warp)
    FunctionGenerator fGenSize {FunctionGenerator::Sine,  1.0f, 48.0f, 2.3f};
    FunctionGenerator fGenX    {FunctionGenerator::Sine, -96.0f, 96.0f, 2.7f};
    FunctionGenerator fGenY    {FunctionGenerator::Sine, -96.0f, 96.0f, 1.7f};
    FunctionGenerator fGenWarp {FunctionGenerator::Sine,  1.0f, 100.0f, 3.7f};

public:
    explicit Fisheye(float amp = 0.5f);

    void SetPosition(Vector2D ofs);
    void SetAmplitude(float amp);

    // New API (replaces ApplyEffect)
    void Apply(IPixelGroup* pixelGroup) override;

    PTX_BEGIN_FIELDS(Fisheye)
        /* TODO: PTX_FIELD(Fisheye, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Fisheye)
        PTX_METHOD_AUTO(Fisheye, SetPosition, "Set position"),
        PTX_METHOD_AUTO(Fisheye, SetAmplitude, "Set amplitude"),
        PTX_METHOD_AUTO(Fisheye, Apply, "Apply")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Fisheye)
        PTX_CTOR(Fisheye, float)
    PTX_END_DESCRIBE(Fisheye)
    
};
