// indexgroup.cpp
#include "indexgroup.hpp"
#include "../../core/utils/casthelper.hpp"

/**
 * @file indexgroup.cpp
 * @brief Implementation of IndexGroup arithmetic and formatting.
 * @date 8/18/2025
 * @author Coela
 */

IndexGroup::IndexGroup() : A(0), B(0), C(0) {}

IndexGroup::IndexGroup(const IndexGroup& indexGroup) : A(indexGroup.A), B(indexGroup.B), C(indexGroup.C) {}

IndexGroup::IndexGroup(uint16_t A, uint16_t B, uint16_t C) : A(A), B(B), C(C) {}

IndexGroup IndexGroup::Add(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) + CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) + CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) + CastHelper::ToU32(indexGroup.C))
    };
}

IndexGroup IndexGroup::Subtract(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) - CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) - CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) - CastHelper::ToU32(indexGroup.C))
    };
}

IndexGroup IndexGroup::Multiply(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) * CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) * CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) * CastHelper::ToU32(indexGroup.C))
    };
}

IndexGroup IndexGroup::Divide(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(CastHelper::ToU32(this->A) / CastHelper::ToU32(indexGroup.A)),
        CastHelper::ToU16(CastHelper::ToU32(this->B) / CastHelper::ToU32(indexGroup.B)),
        CastHelper::ToU16(CastHelper::ToU32(this->C) / CastHelper::ToU32(indexGroup.C))
    };
}

ptx::UString IndexGroup::ToString() {
    ptx::UString sa = Mathematics::DoubleToCleanString(A);
    ptx::UString sb = Mathematics::DoubleToCleanString(B);
    ptx::UString sc = Mathematics::DoubleToCleanString(C);
    
    return "[" + sa + ", " + sb + ", " + sc + "]";
}
