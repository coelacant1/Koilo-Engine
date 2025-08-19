#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/normalparams.hpp"
#include "../../shader/implementations/normalshader.hpp"

/**
 * @file normalmaterial.hpp
 * @brief Alias material pairing NormalParams with NormalShader.
 */

/**
 * @typedef NormalMaterial
 * @brief MaterialT specialization combining NormalParams and NormalShader.
 */
using NormalMaterial = MaterialT<NormalParams, NormalShader>;