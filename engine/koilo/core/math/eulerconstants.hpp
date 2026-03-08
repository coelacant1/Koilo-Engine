// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file EulerConstants.h
 * @brief Provides predefined constants for common Euler rotation orders.
 *
 * The `EulerConstants` namespace defines a collection of commonly used Euler rotation orders,
 * split into static (inertial) and rotating (non-inertial) reference frames.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "eulerorder.hpp"
#include "vector3d.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @namespace EulerConstants
 * @brief A collection of predefined constants for Euler rotation orders.
 *
 * The `EulerConstants` namespace contains a variety of `EulerOrder` configurations
 * commonly used in 3D rotations. These orders are grouped into two categories:
 * - Static frame of reference (inertial).
 * - Rotating frame of reference (non-inertial).
 */
namespace EulerConstants {

// Static frame of reference, inertial reference frame
extern const EulerOrder EulerOrderXYZS; ///< Order: X -> Y -> Z, static frame.
extern const EulerOrder EulerOrderXZYS; ///< Order: X -> Z -> Y, static frame.
extern const EulerOrder EulerOrderYZXS; ///< Order: Y -> Z -> X, static frame.
extern const EulerOrder EulerOrderYXZS; ///< Order: Y -> X -> Z, static frame.
extern const EulerOrder EulerOrderZXYS; ///< Order: Z -> X -> Y, static frame.
extern const EulerOrder EulerOrderZYXS; ///< Order: Z -> Y -> X, static frame.

// Rotating frame of reference, non-inertial reference frame
extern const EulerOrder EulerOrderZYXR; ///< Order: Z -> Y -> X, rotating frame.
extern const EulerOrder EulerOrderYZXR; ///< Order: Y -> Z -> X, rotating frame.
extern const EulerOrder EulerOrderXZYR; ///< Order: X -> Z -> Y, rotating frame.
extern const EulerOrder EulerOrderZXYR; ///< Order: Z -> X -> Y, rotating frame.
extern const EulerOrder EulerOrderYXZR; ///< Order: Y -> X -> Z, rotating frame.
extern const EulerOrder EulerOrderXYZR; ///< Order: X -> Y -> Z, rotating frame.

/**
 * @class EulerConstantsWrapper
 * @brief Runtime reflection wrapper for Euler constants.
 * Note: Classes in nested namespaces are excluded from reflection to avoid code generation issues
 */
class EulerConstantsWrapper {
public:
    EulerConstantsWrapper() = default;

    static const EulerOrder& GetXYZS() { return EulerOrderXYZS; }
    static const EulerOrder& GetXZYS() { return EulerOrderXZYS; }
    static const EulerOrder& GetYZXS() { return EulerOrderYZXS; }
    static const EulerOrder& GetYXZS() { return EulerOrderYXZS; }
    static const EulerOrder& GetZXYS() { return EulerOrderZXYS; }
    static const EulerOrder& GetZYXS() { return EulerOrderZYXS; }
    static const EulerOrder& GetZYXR() { return EulerOrderZYXR; }
    static const EulerOrder& GetYZXR() { return EulerOrderYZXR; }
    static const EulerOrder& GetXZYR() { return EulerOrderXZYR; }
    static const EulerOrder& GetZXYR() { return EulerOrderZXYR; }
    static const EulerOrder& GetYXZR() { return EulerOrderYXZR; }
    static const EulerOrder& GetXYZR() { return EulerOrderXYZR; }

    KL_BEGIN_FIELDS(EulerConstantsWrapper)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(EulerConstantsWrapper)
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetXYZS, "Get xyzs"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetXZYS, "Get xzys"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetYZXS, "Get yzxs"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetYXZS, "Get yxzs"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetZXYS, "Get zxys"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetZYXS, "Get zyxs"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetZYXR, "Get zyxr"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetYZXR, "Get yzxr"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetXZYR, "Get xzyr"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetZXYR, "Get zxyr"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetYXZR, "Get yxzr"),
        KL_SMETHOD_AUTO(EulerConstantsWrapper::GetXYZR, "Get xyzr")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(EulerConstantsWrapper)
        KL_CTOR0(EulerConstantsWrapper)
    KL_END_DESCRIBE(EulerConstantsWrapper)

};

} // namespace EulerConstants

} // namespace koilo
