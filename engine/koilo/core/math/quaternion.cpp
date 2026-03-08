// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/quaternion.hpp>
#include <cmath>


namespace koilo {

// Rotate vector (2D)
Vector2D koilo::Quaternion::RotateVector(const Vector2D& v) const {
    if (IsClose(Quaternion(), Mathematics::EPSILON)) return v;
		
    Quaternion q = UnitQuaternion();

    float s2 = q.W * 2.0f;
    float dPUV = (q.X * v.X + q.Y * v.Y) * 2.0f;
    float dPUU = q.W * q.W - (q.X * q.X + q.Y * q.Y + q.Z * q.Z);

    return Vector2D{
        X * dPUV + v.X * dPUU + (-(q.Z * v.Y)) * s2,
        Y * dPUV + v.Y * dPUU + ((q.Z * v.X)) * s2
    };
}

// Rotate vector with a unit quaternion
Vector2D koilo::Quaternion::RotateVectorUnit(const Vector2D& v, const Quaternion& q) const {
    if (IsClose(Quaternion(), Mathematics::EPSILON)) return v;

    float s2 = q.W * 2.0f;
    float dPUV = (q.X * v.X + q.Y * v.Y) * 2.0f;
    float dPUU = q.W * q.W - (q.X * q.X + q.Y * q.Y + q.Z * q.Z);

    return Vector2D{
        X * dPUV + v.X * dPUU + (-(q.Z * v.Y)) * s2,
        Y * dPUV + v.Y * dPUU + ((q.Z * v.X)) * s2
    };
}

// Unrotate vector (2D)
Vector2D koilo::Quaternion::UnrotateVector(const Vector2D& coordinate) const {
    if (IsClose(Quaternion(), Mathematics::EPSILON)) return coordinate;
    return Conjugate().RotateVector(coordinate);
}

// Unrotate vector (3D)
Vector3D koilo::Quaternion::UnrotateVector(const Vector3D& coordinate) const {
    if (IsClose(Quaternion(), Mathematics::EPSILON)) return coordinate;
    return UnitQuaternion().Conjugate().RotateVector(coordinate);
}

// Get Bivector
Vector3D koilo::Quaternion::GetBiVector() const {
    return Vector3D{ this->X, this->Y, this->Z };
}

Vector3D koilo::Quaternion::GetNormal() const {
    return this->RotateVector(Vector3D(0, 0, 1.0f));
}

// Spherical interpolation
Quaternion koilo::Quaternion::SphericalInterpolation(const Quaternion& q1, const Quaternion& q2, const float& ratio) {
    if (ratio <= Mathematics::EPSILON) return q1;
    if (ratio >= 1.0f - Mathematics::EPSILON) return q2; 

    Quaternion q1U = q1;
    Quaternion q2U = q2;

    q1U = q1U.UnitQuaternion();
    q2U = q2U.UnitQuaternion();

    float dot = q1U.DotProduct(q2U);//Cosine between the two quaternions

    if (dot < 0.0f){//Shortest path correction
        q1U = q1U.AdditiveInverse();
        dot = -dot;
    }

    if (dot > 0.999f){//Linearly interpolates if results are close
        return (q1U.Add( (q2U.Subtract(q1U)).Multiply(ratio) )).UnitQuaternion();
    }
    else
    {
        dot = Mathematics::Constrain<float>(dot, -1, 1);

        float theta0 = acosf(dot);
        float theta = theta0 * ratio;

        //Quaternion q3 = (q2.Subtract(q1.Multiply(dot))).UnitQuaternion();//UQ for orthonomal 
        float f1 = cosf(theta) - dot * sinf(theta) / sinf(theta0);
        float f2 = sinf(theta) / sinf(theta0);

        return q1U.Multiply(f1).Add(q2U.Multiply(f2)).UnitQuaternion();
    }
}

// Delta rotation
Quaternion koilo::Quaternion::DeltaRotation(const Vector3D& angularVelocity, const float& timeDelta) const {
    Quaternion current = Quaternion(this->W, this->X, this->Y, this->Z);
    Vector3D angularVelocityL = angularVelocity;
    Vector3D halfAngle = angularVelocityL * (timeDelta / 2.0f);
    float halfAngleLength = halfAngle.Magnitude();

    if(halfAngleLength > Mathematics::EPSILON){//exponential map
        halfAngle = halfAngle * (sinf(halfAngleLength) / halfAngleLength);
        return (current * Quaternion(cosf(halfAngleLength), halfAngle.X, halfAngle.Y, halfAngle.Z)).UnitQuaternion();
    }
    else{//first taylor series
        return (current * Quaternion(1.0f, halfAngle.X, halfAngle.Y, halfAngle.Z)).UnitQuaternion();
    }
}

// Divide quaternion
Quaternion koilo::Quaternion::Divide(const Quaternion& quaternion) const {
    if(quaternion.IsClose(Quaternion(), Mathematics::EPSILON)) return Quaternion(W, X, Y, Z);

    return Quaternion(
        (W * quaternion.W - X * quaternion.X - Y * quaternion.Y - Z * quaternion.Z),
        (W * quaternion.X + X * quaternion.W + Y * quaternion.Z - Z * quaternion.Y),
        (W * quaternion.Y - X * quaternion.Z + Y * quaternion.W + Z * quaternion.X),
        (W * quaternion.Z + X * quaternion.Y - Y * quaternion.X + Z * quaternion.W)
    );
}

// Divide by scalar
Quaternion koilo::Quaternion::Divide(const float& scalar) const {
    if (Mathematics::IsClose(scalar, 0.0f, Mathematics::EPSILON)) return Quaternion();
    if (Mathematics::IsClose(scalar, 1.0f, Mathematics::EPSILON)) return Quaternion(W, X, Y, Z);
    
    float invert = 1.0f / scalar;
    return Quaternion(W * invert, X * invert, Y * invert, Z * invert);
}

// Power of quaternion
Quaternion koilo::Quaternion::Power(const Quaternion& exponent) const {
    return Quaternion(
        Mathematics::Pow(W, exponent.W),
        Mathematics::Pow(X, exponent.X),
        Mathematics::Pow(Y, exponent.Y),
        Mathematics::Pow(Z, exponent.Z)
    );
}

// Power with scalar exponent
Quaternion koilo::Quaternion::Power(const float& exponent) const {
    return Quaternion(
        Mathematics::Pow(W, exponent),
        Mathematics::Pow(X, exponent),
        Mathematics::Pow(Y, exponent),
        Mathematics::Pow(Z, exponent)
    );
}

// Permutate quaternion
Quaternion koilo::Quaternion::Permutate(const Vector3D& permutation) const {
    Quaternion q = Quaternion(this->W, this->X, this->Y, this->Z);
    float perm[3];
    perm[(int)permutation.X] = q.X;
    perm[(int)permutation.Y] = q.Y;
    perm[(int)permutation.Z] = q.Z;
    q.X = perm[0]; q.Y = perm[1]; q.Z = perm[2];
    return q;
}

// Multiplicative inverse of quaternion
Quaternion koilo::Quaternion::MultiplicativeInverse() const {
    float norm = Normal();
    if(Mathematics::IsClose(norm, 0.0f, Mathematics::EPSILON)) return Quaternion();
    if(Mathematics::IsClose(norm, 1.0f, Mathematics::EPSILON)) return Conjugate();
    return Conjugate().Multiply(1.0f / (norm * norm));
}

// Check if quaternion is NaN
bool koilo::Quaternion::IsNaN() const {
    return Mathematics::IsNaN(W) || Mathematics::IsNaN(X) || Mathematics::IsNaN(Y) || Mathematics::IsNaN(Z);
}

// Check if quaternion is finite
bool koilo::Quaternion::IsFinite() const {
	return !Mathematics::IsInfinite(W) && !Mathematics::IsInfinite(X) && !Mathematics::IsInfinite(Y) && !Mathematics::IsInfinite(Z);
}

// Check if quaternion is infinite
bool koilo::Quaternion::IsInfinite() const {
	return Mathematics::IsInfinite(W) || Mathematics::IsInfinite(X) || Mathematics::IsInfinite(Y) || Mathematics::IsInfinite(Z);
}

// Check if quaternion is non-zero
bool koilo::Quaternion::IsNonZero() const {
    return W != 0 && X != 0 && Y != 0 && Z != 0;
}

// Check if two quaternions are equal
bool koilo::Quaternion::IsEqual(const Quaternion& quaternion) const {
    return !IsNaN() && !quaternion.IsNaN() &&
        W == quaternion.W &&
        X == quaternion.X &&
        Y == quaternion.Y &&
        Z == quaternion.Z;
}

// Convert quaternion to string
koilo::UString koilo::Quaternion::ToString() const {
    koilo::UString w = Mathematics::DoubleToCleanString(this->W);
    koilo::UString x = Mathematics::DoubleToCleanString(this->X);
    koilo::UString y = Mathematics::DoubleToCleanString(this->Y);
    koilo::UString z = Mathematics::DoubleToCleanString(this->Z);
    
    return "[" + w + ", " + x + ", " + y + ", " + z + "]";
}

// Operator overloads (non-inlined)
bool koilo::Quaternion::operator ==(const Quaternion& quaternion) const {
    return this->IsEqual(quaternion);
}

bool koilo::Quaternion::operator !=(const Quaternion& quaternion) const {
    return !(this->IsEqual(quaternion));
}

Quaternion koilo::Quaternion::operator =(const Quaternion& quaternion) {
    this->W = quaternion.W;
    this->X = quaternion.X;
    this->Y = quaternion.Y;
    this->Z = quaternion.Z;
    return quaternion;
}

Quaternion koilo::Quaternion::operator /(const Quaternion& quaternion) const {
    return Divide(quaternion);
}

Quaternion koilo::Quaternion::operator /(const float& scalar) const {
    return Divide(scalar);
}


// Static function definitions

Quaternion koilo::Quaternion::Add(const Quaternion& q1, const Quaternion& q2) {
    return q1.Add(q2);
}

Quaternion koilo::Quaternion::Subtract(const Quaternion& q1, const Quaternion& q2) {
    return q1.Subtract(q2);
}

Quaternion koilo::Quaternion::Multiply(const Quaternion& q1, const Quaternion& q2) {
    return q1.Multiply(q2);
}

Quaternion koilo::Quaternion::Divide(const Quaternion& q1, const Quaternion& q2) {
    return q1.Divide(q2);
}

Quaternion koilo::Quaternion::Power(const Quaternion& q1, const Quaternion& q2) {
    return q1.Power(q2);
}

float koilo::Quaternion::DotProduct(const Quaternion& q1, const Quaternion& q2) {
    return q1.DotProduct(q2);
}

Quaternion koilo::Quaternion::Power(const Quaternion& quaternion, const float& exponent) {
    return quaternion.Power(exponent);
}

Quaternion koilo::Quaternion::Permutate(const Quaternion& quaternion, const Vector3D& vector) {
    return quaternion.Permutate(vector);
}

Quaternion koilo::Quaternion::Absolute(const Quaternion& quaternion) {
    return quaternion.Absolute();
}

Quaternion koilo::Quaternion::AdditiveInverse(const Quaternion& quaternion) {
    return quaternion.AdditiveInverse();
}

Quaternion koilo::Quaternion::MultiplicativeInverse(const Quaternion& quaternion) {
    return quaternion.MultiplicativeInverse();
}

Quaternion koilo::Quaternion::Conjugate(const Quaternion& quaternion) {
    return quaternion.Conjugate();
}

Quaternion koilo::Quaternion::UnitQuaternion(const Quaternion& quaternion) {
    return quaternion.UnitQuaternion();
}

float koilo::Quaternion::Magnitude(const Quaternion& quaternion) {
    return quaternion.Magnitude();
}

float koilo::Quaternion::Normal(const Quaternion& quaternion) {
    return quaternion.Normal();
}

Quaternion koilo::Quaternion::LookAt(const Vector3D& forward, const Vector3D& up) {
    // Negate forward: camera convention uses -Z as the look direction
    Vector3D f = Vector3D(0,0,0).Subtract(forward).UnitSphere();

    // Build orthonormal basis: right = up × f, then recompute up = f × right
    Vector3D r = Vector3D::CrossProduct(up, f).UnitSphere();
    Vector3D u = Vector3D::CrossProduct(f, r);

    // Convert 3×3 rotation matrix (columns: r, u, f) to quaternion
    // Using Shepperd's method for numerical stability
    float m00 = r.X, m01 = u.X, m02 = f.X;
    float m10 = r.Y, m11 = u.Y, m12 = f.Y;
    float m20 = r.Z, m21 = u.Z, m22 = f.Z;

    float trace = m00 + m11 + m22;
    Quaternion q;

    if (trace > 0.0f) {
        float s = Mathematics::Sqrt(trace + 1.0f) * 2.0f;
        q.W = 0.25f * s;
        q.X = (m21 - m12) / s;
        q.Y = (m02 - m20) / s;
        q.Z = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        float s = Mathematics::Sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q.W = (m21 - m12) / s;
        q.X = 0.25f * s;
        q.Y = (m01 + m10) / s;
        q.Z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = Mathematics::Sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q.W = (m02 - m20) / s;
        q.X = (m01 + m10) / s;
        q.Y = 0.25f * s;
        q.Z = (m12 + m21) / s;
    } else {
        float s = Mathematics::Sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q.W = (m10 - m01) / s;
        q.X = (m02 + m20) / s;
        q.Y = (m12 + m21) / s;
        q.Z = 0.25f * s;
    }

    return q.UnitQuaternion();
}

} // namespace koilo
