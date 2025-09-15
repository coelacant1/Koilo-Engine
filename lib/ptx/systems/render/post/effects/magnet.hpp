#pragma once

#include "../effect.hpp"
#include "../../core/ipixelgroup.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/signal/functiongenerator.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @brief Magnetic “pull/warp” distortion using inverse-distance falloff.
 *
 * Amplitude and center can animate via internal FunctionGenerators.
 * Uses PixelGroup adjacency queries to fetch displaced samples into a buffer,
 * then commits the result back to the color array.
 */
class Magnet : public Effect {
private:
    Vector2D offset_{0.0f, 0.0f};
    float    amplitude_ = 0.5f;

    // anim sources (same spirit as your original)
    FunctionGenerator fGenSize_ { FunctionGenerator::Sine, 1.0f, 5000.0f, 2.3f };
    FunctionGenerator fGenX_    { FunctionGenerator::Sine, -96.0f,    96.0f, 2.7f };
    FunctionGenerator fGenY_    { FunctionGenerator::Sine, -96.0f,    96.0f, 1.7f };
    FunctionGenerator fGenWarp_ { FunctionGenerator::Sine, 1.0f,      100.0f, 3.7f };

public:
    explicit Magnet(float amplitude = 0.5f);

    // configuration
    void SetPosition(Vector2D offset);
    void SetAmplitude(float amplitude);

    // effect
    void Apply(IPixelGroup* pixelGroup) override;

    PTX_BEGIN_FIELDS(Magnet)
        /* TODO: PTX_FIELD(Magnet, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Magnet)
        PTX_METHOD_AUTO(Magnet, SetPosition, "Set position"),
        PTX_METHOD_AUTO(Magnet, SetAmplitude, "Set amplitude"),
        PTX_METHOD_AUTO(Magnet, Apply, "Apply")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Magnet)
        PTX_CTOR(Magnet, float)
    PTX_END_DESCRIBE(Magnet)
    
};
