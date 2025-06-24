#include "../../../include/um3d/assets/mesh/indexgroup.hpp"

IndexGroup::IndexGroup() : A(0), B(0), C(0) {}

IndexGroup::IndexGroup(const IndexGroup& indexGroup) : A(indexGroup.A), B(indexGroup.B), C(indexGroup.C) {}

IndexGroup::IndexGroup(uint16_t X, uint16_t Y, uint16_t Z) : A(X), B(Y), C(Z) {}

IndexGroup IndexGroup::Add(IndexGroup indexGroup) {
    return IndexGroup {
        this->A + indexGroup.A,
        this->B + indexGroup.B,
        this->C + indexGroup.C
    };
}

IndexGroup IndexGroup::Subtract(IndexGroup indexGroup) {
    return IndexGroup {
        this->A - indexGroup.A,
        this->B - indexGroup.B,
        this->C - indexGroup.C
    };
}

IndexGroup IndexGroup::Multiply(IndexGroup indexGroup) {
    return IndexGroup {
        this->A * indexGroup.A,
        this->B * indexGroup.B,
        this->C * indexGroup.C
    };
}

IndexGroup IndexGroup::Divide(IndexGroup indexGroup) {
    return IndexGroup {
        this->A / indexGroup.A,
        this->B / indexGroup.B,
        this->C / indexGroup.C
    };
}

string_t IndexGroup::ToString() {
    return "[" + String(A) + ", " + String(B) + ", " + String(C) + "]";
}
