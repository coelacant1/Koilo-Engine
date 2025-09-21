// textbuildershader.hpp
#pragma once

#include <cmath>
#include <cstdint>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "textparams.hpp"

#include "../../../../assets/font/characters.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/rotation.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file textbuildershader.hpp
 * @brief Shader that renders fixed-grid text using 8×8 bitmap glyphs in a 10×10 cell.
 *
 * Each character occupies a 10×10 cell: a 1-pixel border (top/bottom/left/right) and an
 * 8×8 interior for the glyph from @ref Characters. Blink/invert behavior is controlled by
 * the character code and @ref TextBuilderParamsT timing fields. If a fill sub-material is
 * present, glyph pixels are shaded via that material’s shader; otherwise @c fgWhite is used.
 *
 * @tparam lineCount Number of text rows.
 * @tparam charWidth Number of characters per row.
 */
template <uint8_t lineCount, uint8_t charWidth>
class TextBuilderShaderT final : public IShader {
public:
    /**
     * @brief Shade a sample in text canvas space.
     * @param sp Surface properties (position used; normal/uvw passed through to sub-material).
     * @param m  Bound material, expected to be MaterialT<TextBuilderParamsT<lineCount,charWidth>, TextBuilderShaderT<...>>.
     * @return RGB color based on glyph bitmap, blink/invert rule, and optional fill material.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using Mat = MaterialT<TextBuilderParamsT<lineCount, charWidth>,
                              TextBuilderShaderT<lineCount, charWidth>>;
        const auto& P = m.As<Mat>();

        // Coordinate transform
        Vector3D p = sp.position;

        if (P.rotationAngleDeg != 0.0f && !P.isEfficient) {
            p = p - Vector3D(P.rotationOffset.X, P.rotationOffset.Y, 0.0f);
            Quaternion q = Rotation(EulerAngles(Vector3D(0,0,P.rotationAngleDeg),
                                                EulerConstants::EulerOrderXYZS)).GetQuaternion();
            p = q.RotateVector(p);
            p = p + Vector3D(P.rotationOffset.X, P.rotationOffset.Y, 0.0f);
        } else if (P.rotationAngleDeg != 0.0f) {
            // Efficient 90/180/270 approximation
            Vector2D t = Vector2D(p);
            int ang = (int)P.rotationAngleDeg % 360;
            if (Mathematics::IsClose((float)ang, 90.0f, 45.0f)) {
                p.X =  t.Y;  p.Y = -t.X;
            } else if (Mathematics::IsClose((float)ang, 180.0f, 45.0f)) {
                p.X = -t.X;  p.Y = -t.Y;
            } else if (Mathematics::IsClose((float)ang, 270.0f, 45.0f)) {
                p.X = -t.Y;  p.Y =  t.X;
            }
        }

        // Apply position offset
        p = p - Vector3D(P.positionOffset.X, P.positionOffset.Y, 0.0f);

        // Map into character grid; each cell is 10×10 (outer border 1px)
        int x = (int)std::floor(Mathematics::Map(p.X, 0.0f, P.size.X, float(charWidth)*10.0f, 0.0f));
        int y = (int)std::floor(Mathematics::Map(p.Y, 0.0f, P.size.Y, float(lineCount)*10.0f, 0.0f));

        if (x < 0 || x >= charWidth * 10 || y < 0 || y >= lineCount * 10) {
            return P.bgBlack;
        }

        // Bit location inside 10×10 cell
        const uint8_t charXBit = 9 - (x % 10); // 9..0, 0 is left margin
        const uint8_t charYBit = y % 10;       // 0..9, 0/9 are margins

        const char ch = P.lines[y / 10][x / 10];

        // Blink phase
        bool blinkOn = false;
        if (P.blinkPeriodMs > 0) {
            float cycle = std::fmod(P.elapsedMs, float(P.blinkPeriodMs) * 2.0f);
            blinkOn = (cycle > float(P.blinkPeriodMs));
        }

        // Margins
        if (charYBit == 0 || charYBit == 9 || charXBit == 0 || charXBit == 9) {
            if (ch > 90 && blinkOn) {
                return Hue(SampleFill(sp, m, P), 180.0f);
            }
            return P.bgBlack;
        }

        // Glyph interior: 8×8 bits indexed by (charYBit-1, charXBit-1)
        const uint8_t rowByte = Characters::GetCharacter(ch)[charYBit - 1];
        const bool    bitOn   = (rowByte >> (charXBit - 1)) & 0x1;

        if (ch > 90 && blinkOn) {
            // inverted blink
            return bitOn ? P.bgBlack : Hue(SampleFill(sp, m, P), 180.0f);
        } else if (ch > 90) {
            // inverted static
            return bitOn ? Hue(SampleFill(sp, m, P), 180.0f) : P.bgBlack;
        } else {
            // normal
            return bitOn ? SampleFill(sp, m, P) : P.bgBlack;
        }
    }

private:
    /** @brief Hue-rotate a color by @p deg degrees. */
    static RGBColor Hue(const RGBColor& c, float deg) {
        return RGBColor(c).HueShift(deg);
    }

    /**
     * @brief Foreground color sampling for glyph pixels.
     *
     * If a fill material is provided, calls its shader with a local SurfaceProperties
     * constructed from @p sp (position/normal/uvw reused). Otherwise returns @c fgWhite.
     */
    static RGBColor SampleFill(const SurfaceProperties& sp,
                               const IMaterial& /*self*/,
                               const TextBuilderParamsT<lineCount, charWidth>& P)
    {
        if (!P.fillMat) return P.fgWhite;

        const IShader* sh = P.fillMat->GetShader();
        if (!sh) return P.fgWhite;

        Vector3D pos = sp.position;
        Vector3D nrm = sp.normal;
        Vector3D uvw = sp.uvw;
        SurfaceProperties sp2{pos, nrm, uvw};

        return sh->Shade(sp2, *P.fillMat);
    }

};
