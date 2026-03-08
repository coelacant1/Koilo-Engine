// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/rotationmatrix.hpp>
#include <cmath>


namespace koilo {

Vector3D koilo::RotationMatrix::ConvertCoordinateToVector() {
    if (didRotate) {
        return Vector3D((XAxis.X + YAxis.X + ZAxis.X), (XAxis.Y + YAxis.Y + ZAxis.Y), (XAxis.Z + YAxis.Z + ZAxis.Z));
    }
    else {
        return InitialVector;
    }
}

void koilo::RotationMatrix::ReadjustMatrix() {
    float X = (XAxis.X + YAxis.X + ZAxis.X);
    float Y = (XAxis.Y + YAxis.Y + ZAxis.Y);
    float Z = (XAxis.Z + YAxis.Z + ZAxis.Z);

    XAxis = Vector3D(X, X, X);
    YAxis = Vector3D(Y, Y, Y);
    ZAxis = Vector3D(Z, Z, Z);
}

Vector3D koilo::RotationMatrix::Rotate(Vector3D rotation) {
    if (rotation.X != 0) {
        RotateX(rotation.X);
        didRotate = true;

        if (rotation.Y != 0 || rotation.Z != 0) {
            ReadjustMatrix();
        }
    }

    if (rotation.Y != 0) {
        RotateY(rotation.Y);
        didRotate = true;

        if (rotation.Z != 0) {
            ReadjustMatrix();
        }
    }

    if (rotation.Z != 0) {
        RotateZ(rotation.Z);
        didRotate = true;
    }

    return ConvertCoordinateToVector();
}

Vector3D koilo::RotationMatrix::RotateX(float theta) {
    float c = cosf(Mathematics::DegreesToRadians(theta));
    float s = sinf(Mathematics::DegreesToRadians(theta));

    XAxis = Vector3D(1, 0,  0).Multiply(XAxis);
    YAxis = Vector3D(0, c, -s).Multiply(YAxis);
    ZAxis = Vector3D(0, s,  c).Multiply(ZAxis);
    
    return ConvertCoordinateToVector();
}

Vector3D koilo::RotationMatrix::RotateY(float theta) {
    float c = cosf(Mathematics::DegreesToRadians(theta));
    float s = sinf(Mathematics::DegreesToRadians(theta));

    XAxis = Vector3D( c, 0, s).Multiply(XAxis);
    YAxis = Vector3D( 0, 1, 0).Multiply(YAxis);
    ZAxis = Vector3D(-s, 0, c).Multiply(ZAxis);
    
    return ConvertCoordinateToVector();
}

Vector3D koilo::RotationMatrix::RotateZ(float theta) {
    float c = cosf(Mathematics::DegreesToRadians(theta));
    float s = sinf(Mathematics::DegreesToRadians(theta));

    XAxis = Vector3D(c, -s, 0).Multiply(XAxis);
    YAxis = Vector3D(s,  c, 0).Multiply(YAxis);
    ZAxis = Vector3D(0,  0, 1).Multiply(ZAxis);
    
    return ConvertCoordinateToVector();
}

void koilo::RotationMatrix::RotateRelative(RotationMatrix rM) {
    Multiply(rM);
}

koilo::RotationMatrix::RotationMatrix() {
    XAxis = Vector3D(1.0f, 0.0f, 0.0f);
    YAxis = Vector3D(0.0f, 1.0f, 0.0f);
    ZAxis = Vector3D(0.0f, 0.0f, 1.0f);
}

koilo::RotationMatrix::RotationMatrix(Vector3D axes) {
    XAxis = Vector3D(axes.X, axes.X, axes.X);
    YAxis = Vector3D(axes.Y, axes.Y, axes.Y);
    ZAxis = Vector3D(axes.Z, axes.Z, axes.Z);
}

koilo::RotationMatrix::RotationMatrix(Vector3D X, Vector3D Y, Vector3D Z) {
    XAxis = X;
    YAxis = Y;
    ZAxis = Z;
}

RotationMatrix koilo::RotationMatrix::Multiply(float d) {
    return RotationMatrix {
        XAxis.Multiply(d),
        YAxis.Multiply(d),
        ZAxis.Multiply(d)
    };
}

RotationMatrix koilo::RotationMatrix::Multiply(RotationMatrix rM) {
    return RotationMatrix {
        XAxis.Multiply(rM.XAxis),
        YAxis.Multiply(rM.YAxis),
        ZAxis.Multiply(rM.ZAxis)
    };
}

RotationMatrix koilo::RotationMatrix::Normalize() {
    Vector3D vz = Vector3D::CrossProduct(XAxis, YAxis);
    Vector3D vy = Vector3D::CrossProduct(vz, XAxis);

    return RotationMatrix {
        XAxis.UnitSphere(),
        vy.UnitSphere(),
        vz.UnitSphere()
    };
}

RotationMatrix koilo::RotationMatrix::Transpose() {
    XAxis = Vector3D(XAxis.X, YAxis.X, ZAxis.X);
    YAxis = Vector3D(XAxis.Y, YAxis.Y, ZAxis.Y);
    ZAxis = Vector3D(XAxis.Z, YAxis.Z, ZAxis.Z);

    return *this;
}

RotationMatrix koilo::RotationMatrix::Inverse() {
    RotationMatrix rM = RotationMatrix{
        Vector3D::CrossProduct(YAxis, ZAxis),
        Vector3D::CrossProduct(ZAxis, XAxis),
        Vector3D::CrossProduct(XAxis, YAxis)
    };
    
    rM = Transpose().Multiply(1 / rM.Determinant());

    Transpose();
    return Multiply(1 / Determinant());
}

bool koilo::RotationMatrix::IsEqual(RotationMatrix rM) {
    return XAxis.IsEqual(rM.XAxis) && YAxis.IsEqual(rM.YAxis) && ZAxis.IsEqual(rM.ZAxis);
}

float koilo::RotationMatrix::Determinant() {
    return XAxis.X * (YAxis.Y * ZAxis.Z - ZAxis.Y * YAxis.Z) -
        YAxis.X * (ZAxis.Z * XAxis.Y - ZAxis.Y * XAxis.Z) +
        ZAxis.X * (XAxis.Y * YAxis.Z - YAxis.Y * XAxis.Z);
}

Vector3D koilo::RotationMatrix::RotateVector(Vector3D rotate, Vector3D coordinates) {
    //calculate rotation matrix
    RotationMatrix matrix = RotationMatrix(coordinates);

    matrix.Rotate(rotate);

    if (rotate.X == 0 && rotate.Y == 0 && rotate.Z == 0) {
        return coordinates;
    }
    else {
        return matrix.ConvertCoordinateToVector();
    }
}

koilo::UString koilo::RotationMatrix::ToString() {
    koilo::UString x = XAxis.ToString();
    koilo::UString y = YAxis.ToString();
    koilo::UString z = ZAxis.ToString();

    return x + "\n" + y + "\n" + z + "\n";
}

RotationMatrix koilo::RotationMatrix::operator =(RotationMatrix rM) {
    this->XAxis = rM.XAxis;
    this->YAxis = rM.YAxis;
    this->ZAxis = rM.ZAxis;

    return *this;
}

} // namespace koilo
