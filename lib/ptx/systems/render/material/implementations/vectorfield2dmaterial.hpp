#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../shader/implementations/vectorfield2dparams.hpp"
#include "../../shader/implementations/vectorfield2dshader.hpp"

/**
 * @file vectorfield2dmaterial.hpp
 * @brief Material wrapper that edits a 2D vector-field parameter block and renders via VectorField2DShader.
 *
 * Provides UI-style setters for transform/state and several simple simulation/authoring operations
 * (boundary, diffusion, advection, and pattern generators).
 */

/**
 * @class VectorField2DMaterial
 * @brief Edits @ref VectorField2DParams and shades with @ref VectorField2DShader.
 *
 * Inherits @ref MaterialT so that the parameter block is publicly accessible; methods modify
 * buffers such as @c vecX, @c vecY, @c vecD, and @c vecDP that live in @ref VectorField2DParams.
 */
class VectorField2DMaterial
: public MaterialT<VectorField2DParams, VectorField2DShader> {
    using Base = MaterialT<VectorField2DParams, VectorField2DShader>;
public:
    /**
     * @brief Construct with grid dimensions forwarded to the parameter block.
     * @param x Grid width  (cells in X).
     * @param y Grid height (cells in Y).
     */
    explicit VectorField2DMaterial(uint16_t x, uint16_t y)
    : Base(x, y) {}

    // ----- Transform / display state -----

    /** @brief Set logical size (used for mapping cell coordinates). */
    void SetSize(float sx, float sy)              { this->size = {sx, sy}; }

    /** @brief Set world position/center. */
    void SetPosition(float px, float py)          { this->position = {px, py}; }

    /** @brief Set rotation in degrees. */
    void SetRotation(float deg)                   { this->rotation = deg; }

    /** @brief Show scalar density. */
    void RenderDensity()                          { this->showDensity = true; }

    /** @brief Show vector field (arrows/lines). */
    void RenderVector()                           { this->showDensity = false; }

    /** @brief Grid width (cells). */
    uint16_t GetCountX() const { return this->countX; }

    /** @brief Grid height (cells). */
    uint16_t GetCountY() const { return this->countY; }

    // ----- Simulation / authoring ops -----

    /**
     * @brief Boundary processing hook (placeholder like legacy).
     * @note Implement as needed to enforce edge conditions.
     */
    void Boundary() {
        // placeholder like legacy
    }

    /**
     * @brief Diffuse density using a Jacobi-style relaxation sweep.
     * @param viscosity Diffusion coefficient.
     * @param dt        Time step.
     *
     * Reads previous density @c vecDP and writes updated density @c vecD.
     * Interior cells are relaxed toward the average of their Von Neumann neighbors.
     */
    void Diffuse(float viscosity, float dt) {
        const float a  = dt * viscosity * float(this->countX) * float(this->countY);
        const float aS = 1.0f + (4.0f * a);
        const uint32_t total = uint32_t(this->countX) * uint32_t(this->countY);

        for (uint32_t i = 0; i < total; ++i) {
            this->vecDP[i] = Mathematics::Constrain(int(this->vecD[i]) - 1, 0, 127);
        }

        for (int x = 1; x < this->countX - 1; ++x) {
            for (int y = 1; y < this->countY - 1; ++y) {
                const uint32_t idx = x + this->countX * y;
                const uint32_t top = x + this->countX * (y - 1);
                const uint32_t bot = x + this->countX * (y + 1);
                const uint32_t rgt = idx + 1;
                const uint32_t lft = idx - 1;

                const float v = ( float(this->vecDP[idx]) +
                                  a * ( float(this->vecD[top]) + float(this->vecD[bot]) +
                                        float(this->vecD[rgt]) + float(this->vecD[lft]) ) ) / aS;
                this->vecD[idx] = int8_t(v);
            }
        }
    }

    /**
     * @brief Semi-Lagrangian advection of density along the velocity field.
     * @param dt Time step.
     *
     * Back-traces from each cell by @c (vecX, vecY), bilinearly samples @c vecDP, and writes @c vecD.
     */
    void Advect(float dt) {
        int x0, x1, y0, y1;
        float s0, s1, t0, t1;

        const float dtX = dt * float(this->countX);
        const float dtY = dt * float(this->countY);

        for (int x = 1; x < this->countX - 1; ++x) {
            for (int y = 1; y < this->countY - 1; ++y) {
                const uint32_t index = x + this->countX * y;
                float xA = float(x) - dtX * float(this->vecX[index]);
                float yA = float(y) - dtY * float(this->vecY[index]);

                if (xA < 0.5f)                      xA = 0.5f;
                if (xA > float(this->countX)-0.5f)  xA = float(this->countX)-0.5f;
                x0 = int(xA); x1 = x0 + 1;

                if (yA < 0.5f)                      yA = 0.5f;
                if (yA > float(this->countY)-0.5f)  yA = float(this->countY)-0.5f;
                y0 = int(yA); y1 = y0 + 1;

                s1 = xA - float(x0); s0 = 1.0f - s1;
                t1 = yA - float(y0); t0 = 1.0f - t1;

                const auto clampIdx = [&](uint32_t v) {
                    return Mathematics::Constrain(v, uint32_t(0), index);
                };

                const float td00 = t0 * float(this->vecDP[clampIdx(uint32_t(x0 + this->countX * y0))]);
                const float td01 = t1 * float(this->vecDP[clampIdx(uint32_t(x0 + this->countX * y1))]);
                const float td10 = t0 * float(this->vecDP[clampIdx(uint32_t(x1 + this->countX * y0))]);
                const float td11 = t1 * float(this->vecDP[clampIdx(uint32_t(x1 + this->countX * y1))]);

                const float value = s0 * (td00 + td01) + s1 * (td10 + td11);
                this->vecD[index] = int8_t(value);
            }
        }
    }

    /**
     * @brief Populate a sinusoidal vector/density field.
     * @param ratio     Time/phase ratio.
     * @param period    Spatial period control.
     * @param amplitude Output amplitude (pre-clamped to [-1,1] then scaled to int8).
     */
    void SineField(float ratio, float period, float amplitude) {
        for (int x = 0; x < this->countX; ++x) {
            for (int y = 0; y < this->countY; ++y) {
                const float posX = ((float(x)/float(this->countX) - 0.5f) * 2.0f * this->size.X);
                const float posY = ((float(y)/float(this->countY) - 0.5f) * 2.0f * this->size.Y);
                const uint32_t idx = x + this->countX * y;

                this->vecX[idx] = int8_t(Mathematics::Constrain(sinf((posX + posY) / (period * 6.28f * 1000.0f) + ratio * 6.28f) * amplitude, -1.0f, 1.0f) * 127.0f);
                this->vecY[idx] = int8_t(Mathematics::Constrain(cosf((posX - posY) / (period * 6.28f * 1000.0f) + ratio * 6.28f) * amplitude, -1.0f, 1.0f) * 127.0f);
                this->vecD[idx] = int8_t(Mathematics::Constrain((sinf((posX + posY) / (period * 6.28f * 50.0f)) + cosf((posX - posY) / (period * 6.28f * 50.0f))) * amplitude, -1.0f, 1.0f) * 127.0f);
            }
        }
    }

    /**
     * @brief Checker/step-style vector field with optional density boosts where steps differ.
     * @param ratio     Time/phase ratio.
     * @param period    Step size control.
     * @param intensity Additive density increment when x/y steps differ.
     */
    void StepField(float ratio, float period, float intensity) {
        const float offX = sinf(ratio * 2.0f * Mathematics::MPI * 2.0f) * period;
        const float offY = cosf(ratio * 2.0f * Mathematics::MPI * 2.0f) * period;
        for (int x = 0; x < this->countX; ++x) {
            for (int y = 0; y < this->countY; ++y) {
                float posX = ((float(x)/float(this->countX) - 0.5f) * 2.0f * this->size.X) + offX;
                float posY = ((float(y)/float(this->countY) - 0.5f) * 2.0f * this->size.Y) + offY;
                const uint32_t idx = x + this->countX * y;

                const bool xOdd = (int)(posX * 0.3f / (10.0f / period)) % 2;
                const bool yOdd = (int)(posY * 0.3f / (10.0f / period)) % 2;

                this->vecX[idx] = xOdd ? 127 : -128;
                this->vecY[idx] = yOdd ? 127 : -128;
                if (xOdd != yOdd) this->vecD[idx] = Mathematics::Constrain(int8_t(this->vecD[idx] + intensity), int8_t(0), int8_t(127));
            }
        }
    }

    /**
     * @brief Moving square density pulse.
     * @param ratio     Time/phase ratio.
     * @param period    Square edge length (also used for motion offset).
     * @param intensity Additive density increment inside the square.
     */
    void MovingSquareField(float ratio, float period, float intensity) {
        const float offX = sinf(ratio * 2.0f * Mathematics::MPI * 2.0f) * period;
        const float offY = cosf(ratio * 2.0f * Mathematics::MPI * 2.0f) * period;
        for (int x = 0; x < this->countX; ++x) {
            for (int y = 0; y < this->countY; ++y) {
                float posX = ((float(x)/float(this->countX) - 0.5f) * 2.0f * this->size.X) + offX;
                float posY = ((float(y)/float(this->countY) - 0.5f) * 2.0f * this->size.Y) + offY;
                const uint32_t idx = x + this->countX * y;

                if ( posX < period/2.0f && posX > -period/2.0f &&
                     posY < period/2.0f && posY > -period/2.0f ) {
                    this->vecD[idx] = Mathematics::Constrain(int8_t(this->vecD[idx] + intensity), int8_t(0), int8_t(127));
                }
            }
        }
    }

    /**
     * @brief Spiral vector field oriented by radius.
     * @param period    Angular frequency scale.
     * @param amplitude Output amplitude (pre-clamped to [-1,1] then scaled to int8).
     */
    void SpiralField(float /*ratio*/, float period, float amplitude) {
        for (int x = 0; x < this->countX; ++x) {
            for (int y = 0; y < this->countY; ++y) {
                const float posX = ((float(x)/float(this->countX) - 0.5f) * 2.0f * this->size.X);
                const float posY = ((float(y)/float(this->countY) - 0.5f) * 2.0f * this->size.Y);
                const float magn = sqrtf(posX*posX + posY*posY);
                const uint32_t idx = x + this->countX * y;

                this->vecX[idx] = int8_t(Mathematics::Constrain((posX * cosf(2.0f * magn * period / 40.0f)) * 0.01f * amplitude, -1.0f, 1.0f) * 127.0f);
                this->vecY[idx] = int8_t(Mathematics::Constrain((posY * sinf(2.0f * magn * period / 40.0f)) * 0.01f * amplitude, -1.0f, 1.0f) * 127.0f);
            }
        }
    }
};
