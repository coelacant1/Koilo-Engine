#include <unity.h>
#include "testmathematics.hpp"
#include "testquaternion.hpp"
#include "testrotation.hpp"
#include "testrotationmatrix.hpp"
#include "testvector2d.hpp"
#include "testvector3d.hpp"

void setUp() {}

void tearDown() {}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    TestMathematics::RunAllTests();
    TestQuaternion::RunAllTests();
    TestRotation::RunAllTests();
    TestRotationMatrix::RunAllTests();
    TestVector2D::RunAllTests();
    TestVector3D::RunAllTests();

    UNITY_END();
}
