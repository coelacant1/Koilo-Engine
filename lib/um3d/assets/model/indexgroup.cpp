#include "indexgroup.hpp"

IndexGroup::IndexGroup() : A(0), B(0), C(0) {}

IndexGroup::IndexGroup(const IndexGroup& indexGroup) : A(indexGroup.A), B(indexGroup.B), C(indexGroup.C) {}

IndexGroup::IndexGroup(uint16_t X, uint16_t Y, uint16_t Z) : A(X), B(Y), C(Z) {}

IndexGroup IndexGroup::Add(IndexGroup indexGroup) {
    return IndexGroup {
        CastHelper::ToU16(this->A + indexGroup.A),
        CastHelper::ToU16(this->B + indexGroup.B),
        CastHelper::ToU16(this->C + indexGroup.C)
    };
}

IndexGroup IndexGroup::Subtract(IndexGroup indexGroup) {
    return IndexGroup {
        CastHelper::ToU16(this->A - indexGroup.A),
        CastHelper::ToU16(this->B - indexGroup.B),
        CastHelper::ToU16(this->C - indexGroup.C)
    };
}

IndexGroup IndexGroup::Multiply(IndexGroup indexGroup) {
    return IndexGroup {
        CastHelper::ToU16(this->A * indexGroup.A),
        CastHelper::ToU16(this->B * indexGroup.B),
        CastHelper::ToU16(this->C * indexGroup.C)
    };
}

IndexGroup IndexGroup::Divide(IndexGroup indexGroup) {
    return IndexGroup {
        CastHelper::ToU16(this->A / indexGroup.A),
        CastHelper::ToU16(this->B / indexGroup.B),
        CastHelper::ToU16(this->C / indexGroup.C)
    };
}

um3d::UString IndexGroup::ToString() {

    um3d::UString sa = Mathematics::DoubleToCleanString(A);
    um3d::UString sb = Mathematics::DoubleToCleanString(B);
    um3d::UString sc = Mathematics::DoubleToCleanString(C);


    return "[" + sa + ", " + sb + ", " + sc + "]";
}
