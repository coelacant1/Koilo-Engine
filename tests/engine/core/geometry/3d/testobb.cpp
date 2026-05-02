// SPDX-License-Identifier: GPL-3.0-or-later
#include "testobb.hpp"
#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <cmath>

using namespace koilo;

static Quaternion MakeRotZ(float radians) {
    const float h = radians * 0.5f;
    return Quaternion(std::cos(h), 0.0f, 0.0f, std::sin(h));
}

