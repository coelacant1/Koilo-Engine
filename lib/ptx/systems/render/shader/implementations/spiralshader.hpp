// spiralshader.hpp
#pragma once

#include <cmath>
#include <cstdint>
#include "../../../../registry/reflect_macros.hpp"

#include "../ishader.hpp"
#include "../../material/materialt.hpp"   // IMaterial::As<T>()
#include "spiralparams.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/eulerangles.hpp"
#include "../../../../core/math/eulerconstants.hpp"
#include "../../../../core/math/rotation.hpp"
#include "../../../../core/math/quaternion.hpp"

/**
 * @file spiralshader.hpp
 * @brief Stateless shader that generates a spiral pattern and selects a palette index.
 * @tparam N Number of colors available in the palette.
 */

/**
 * @tparam N Number of colors.
 * @brief Stateless spiral shader that maps surface position to a palette index.
 *
 * Steps:
 * 1. Transform @c surf.position by rotating around @ref SpiralParams::rotationOffset and
 *    translating by @ref SpiralParams::positionOffset.
 * 2. Compute polar coordinates (radius, angle) on the XY plane.
 * 3. Combine @ref SpiralParams::width and @ref SpiralParams::bend to produce a wrapped ratio.
 * 4. Map the ratio to an index in @ref SpiralParams::colors and return the selected color.
 */
template <size_t N>
class SpiralShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface point using a spiral palette mapping.
     * @param surf Surface properties (expects XY in @c surf.position).
     * @param m    Material instance carrying @ref SpiralParams.
     * @return Selected color from the palette.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using MatBase = MaterialT<SpiralParams<N>, SpiralShaderT<N>>;
        const auto& P = m.As<MatBase>();

        // --- position prep on XY plane
        Vector3D pos = surf.position;

        // translate so rotation happens around rotationOffset
        pos = pos - Vector3D(P.rotationOffset.X, P.rotationOffset.Y, 0.0f);

        if (P.rotationAngle != 0.0f) {
            Quaternion q = Rotation(
                EulerAngles(Vector3D(0.0f, 0.0f, P.rotationAngle),
                            EulerConstants::EulerOrderXYZS)
            ).GetQuaternion();
            pos = q.RotateVector(pos);
        }

        // translate back, then apply positionOffset
        pos = pos + Vector3D(P.rotationOffset.X, P.rotationOffset.Y, 0.0f);
        pos = pos - Vector3D(P.positionOffset.X, P.positionOffset.Y, 0.0f);

        // --- spiral coordinates
        const float radius = Vector2D(pos.X, pos.Y).Magnitude();
        const float angle  = std::atan2(pos.Y, pos.X); // [-pi, pi]

        // original: fract(width * angle / MPI + bend * pow(radius, 0.3))
        const float termA = (P.width != 0.0f)
            ? (P.width * angle / Mathematics::MPI)
            : 0.0f;
        const float termB = (P.bend  != 0.0f)
            ? (P.bend * std::pow(radius, 0.3f))
            : 0.0f;

        float ratio = Mathematics::Fract(termA + termB); // [0,1)
        // map to color bucket
        uint8_t idx = static_cast<uint8_t>(std::floor(ratio * float(N)));
        if (idx >= N) idx = N - 1;

        return P.colors[idx];
    }

    /* NOTE: SpiralShaderT is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(SpiralShaderT)
        /* TODO: PTX_FIELD(SpiralShaderT, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(SpiralShaderT)
        PTX_METHOD_AUTO(SpiralShaderT, Shade, "Shade")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(SpiralShaderT)
        /* TODO: PTX_CTOR0(SpiralShaderT) or PTX_CTOR(SpiralShaderT, ...) */
    PTX_END_DESCRIBE(SpiralShaderT)
    
};
