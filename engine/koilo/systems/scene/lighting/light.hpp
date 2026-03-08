// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Light.hpp
 * @brief Defines the Light class for managing light sources in a 3D scene.
 *
 * The Light class encapsulates the properties of a light source, including its position,
 * intensity, falloff, and attenuation curve. It provides methods for manipulating
 * these properties and updating the light's behavior in the scene.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Light
 * @brief Represents a light source with position, intensity, and falloff properties.
 */
class Light {
public:
    /**
     * @brief Default constructor for the Light class.
     */
    Light();

    /**
     * @brief Constructs a Light object with specified properties.
     * @param p Position of the light source.
     * @param intensity Intensity of the light.
     * @param falloff Falloff rate of the light.
     * @param a Curve parameter for attenuation.
     * @param b Curve parameter for attenuation.
     */
    Light(Vector3D p, Vector3D intensity, float falloff, float a, float b);

    /**
     * @brief Sets the light's properties.
     * @param p Position of the light source.
     * @param intensity Intensity of the light.
     * @param falloff Falloff rate of the light.
     * @param a Curve parameter for attenuation.
     * @param b Curve parameter for attenuation.
     */
    void Set(Vector3D p, Vector3D intensity, float falloff, float a, float b);

    /**
     * @brief Sets the intensity of the light.
     * @param intensity Intensity vector of the light.
     */
    void SetIntensity(Vector3D intensity);

    /**
     * @brief Sets the falloff and attenuation parameters for the light.
     * @param falloff Falloff rate of the light.
     * @param a Curve parameter for attenuation.
     * @param b Curve parameter for attenuation.
     */
    void SetFalloff(float falloff, float a, float b);

    /**
     * @brief Moves the light to a specified position.
     * @param p New position of the light source.
     */
    void MoveTo(Vector3D p);

    /**
     * @brief Translates the light by a specified vector.
     * @param p Translation vector.
     */
    void Translate(Vector3D p);

    /**
     * @brief Sets the falloff rate of the light.
     * @param falloff Falloff rate.
     */
    void SetFalloff(float falloff);

    /**
     * @brief Sets the attenuation curve parameters for the light.
     * @param a Curve parameter A.
     * @param b Curve parameter B.
     */
    void SetCurve(float a, float b);

    /**
     * @brief Retrieves the position of the light source.
     * @return Position vector of the light.
     */
    Vector3D GetPosition();

    /**
     * @brief Retrieves the falloff rate of the light.
     * @return Falloff rate.
     */
    float GetFalloff();

    /**
     * @brief Retrieves the first curve parameter for attenuation.
     * @return Curve parameter A.
     */
    float GetCurveA();

    /**
     * @brief Retrieves the second curve parameter for attenuation.
     * @return Curve parameter B.
     */
    float GetCurveB();

    /**
     * @brief Retrieves the intensity of the light.
     * @return Intensity vector.
     */
    Vector3D GetIntensity();

private:
    Vector3D p; ///< Position of the light source.
    Vector3D intensity; ///< Intensity vector of the light.
    float falloff; ///< Falloff rate of the light.
    float a; ///< Attenuation curve parameter A.
    float b; ///< Attenuation curve parameter B.

    KL_BEGIN_FIELDS(Light)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Light)
        KL_METHOD_AUTO(Light, Set, "Set"),
        KL_METHOD_AUTO(Light, SetIntensity, "Set intensity"),
        /* Set falloff */ KL_METHOD_OVLD(Light, SetFalloff, void, float, float, float),
        KL_METHOD_AUTO(Light, MoveTo, "Move to"),
        KL_METHOD_AUTO(Light, Translate, "Translate"),
        /* Set falloff */ KL_METHOD_OVLD(Light, SetFalloff, void, float),
        KL_METHOD_AUTO(Light, SetCurve, "Set curve"),
        KL_METHOD_AUTO(Light, GetPosition, "Get position"),
        KL_METHOD_AUTO(Light, GetFalloff, "Get falloff"),
        KL_METHOD_AUTO(Light, GetCurveA, "Get curve a"),
        KL_METHOD_AUTO(Light, GetCurveB, "Get curve b"),
        KL_METHOD_AUTO(Light, GetIntensity, "Get intensity")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Light)
        KL_CTOR0(Light),
        KL_CTOR(Light, Vector3D, Vector3D, float, float, float)
    KL_END_DESCRIBE(Light)

};

} // namespace koilo
