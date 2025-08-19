// indexgroup.cpp
#include "indexgroup.hpp"

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
        CastHelper::ToU16(this->A + indexGroup.A),
        CastHelper::ToU16(this->B + indexGroup.B),
        CastHelper::ToU16(this->C + indexGroup.C)
    };
}

IndexGroup IndexGroup::Subtract(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(this->A - indexGroup.A),
        CastHelper::ToU16(this->B - indexGroup.B),
        CastHelper::ToU16(this->C - indexGroup.C)
    };
}

IndexGroup IndexGroup::Multiply(IndexGroup indexGroup) {
    return IndexGroup{
        CastHelper::ToU16(this->A * indexGroup.A),
        CastHelper::ToU16(this->B * indexGroup.B),
        CastHelper::ToU16(this->C * indexGroup.C)
    };
}

IndexGroup IndexGroup::Divide(IndexGroup indexGroup) {
    // Caller is responsible for ensuring indexGroup.A/B/C are non-zero.
    return IndexGroup{
        CastHelper::ToU16(this->A / indexGroup.A),
        CastHelper::ToU16(this->B / indexGroup.B),
        CastHelper::ToU16(this->C / indexGroup.C)
    };
}

uc3d::UString IndexGroup::ToString() {
    uc3d::UString sa = Mathematics::DoubleToCleanString(A);
    uc3d::UString sb = Mathematics::DoubleToCleanString(B);
    uc3d::UString sc = Mathematics::DoubleToCleanString(C);
    return "[" + sa + ", " + sb + ", " + sc + "]";
}
