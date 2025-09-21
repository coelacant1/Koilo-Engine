/**
 * @file blendshape.h
 * @brief Declares the Morph class for applying vertex-based transformations to 3D objects.
 *
 * This file defines the Morph class, which manages vertex transformations for
 * 3D objects using weight-based morphing.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include "../../../core/math/mathematics.hpp" // Include for mathematical operations.
#include "../../../assets/model/itrianglegroup.hpp" // Include for rendering triangle groups.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class Morph
 * @brief Handles vertex-based morphing for 3D objects.
 *
 * The Morph class provides functionality to apply transformations to 3D objects
 * by manipulating vertex data based on a weight parameter.
 */
class Blendshape {
private:
    int count = 0; ///< The number of vertices affected by the morph.
    const int* indexes; ///< Pointer to an array of vertex indexes affected by the morph.
    const Vector3D* vertices; ///< Pointer to an array of vertex data for the morph.

public:
    float Weight = 0.0f; ///< The weight of the morph, controlling the intensity of the transformation.

    /**
     * @brief Constructs a Morph object with mutable vertex data.
     *
     * @param count The number of vertices affected by the morph.
     * @param indexes Pointer to an array of vertex indexes.
     * @param vertices Pointer to an array of vertex data.
     */
    Blendshape(int count, int* indexes, Vector3D* vertices);

    /**
     * @brief Constructs a Morph object with immutable vertex data.
     *
     * @param count The number of vertices affected by the morph.
     * @param indexes Pointer to a constant array of vertex indexes.
     * @param vertices Pointer to a constant array of vertex data.
     */
    Blendshape(int count, const int* indexes, const Vector3D* vertices);

    /**
     * @brief Applies the morph transformation to a 3D object.
     *
     * @param obj Pointer to the ITriangleGroup representing the 3D object to morph.
     */
    void BlendObject3D(ITriangleGroup* obj);

    PTX_BEGIN_FIELDS(Blendshape)
        PTX_FIELD(Blendshape, Weight, "Weight", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Blendshape)
        PTX_METHOD_AUTO(Blendshape, BlendObject3D, "Blend object3 d")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Blendshape)
        PTX_CTOR(Blendshape, int, int *, Vector3D *),
        PTX_CTOR(Blendshape, int, const int *, const Vector3D *)
    PTX_END_DESCRIBE(Blendshape)

};
