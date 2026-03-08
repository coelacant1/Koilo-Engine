// SPDX-License-Identifier: GPL-3.0-or-later
// indexgroup.cpp
#include <koilo/assets/model/indexgroup.hpp>
#include <koilo/core/utils/casthelper.hpp>


namespace koilo {

/**
 * @file indexgroup.cpp
 * @brief Implementation of IndexGroup arithmetic and formatting.
 * @date 8/18/2025
 * @author Coela
 */

koilo::IndexGroup::IndexGroup() : A(0), B(0), C(0) {}

koilo::IndexGroup::IndexGroup(const IndexGroup& indexGroup) : A(indexGroup.A), B(indexGroup.B), C(indexGroup.C) {}

koilo::IndexGroup::IndexGroup(uint32_t A, uint32_t B, uint32_t C) : A(A), B(B), C(C) {}

IndexGroup koilo::IndexGroup::Add(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) + CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) + CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) + CastHelper::ToU32(indexGroup.C))
    };
}

IndexGroup koilo::IndexGroup::Subtract(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) - CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) - CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) - CastHelper::ToU32(indexGroup.C))
    };
}

IndexGroup koilo::IndexGroup::Multiply(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) * CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) * CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) * CastHelper::ToU32(indexGroup.C))
    };
}

IndexGroup koilo::IndexGroup::Divide(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) / CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) / CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) / CastHelper::ToU32(indexGroup.C))
    };
}

koilo::UString koilo::IndexGroup::ToString() {
    koilo::UString sa = Mathematics::DoubleToCleanString(A);
    koilo::UString sb = Mathematics::DoubleToCleanString(B);
    koilo::UString sc = Mathematics::DoubleToCleanString(C);
    
    return "[" + sa + ", " + sb + ", " + sc + "]";
}

} // namespace koilo
