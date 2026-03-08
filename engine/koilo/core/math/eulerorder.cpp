// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/eulerorder.hpp>


namespace koilo {

// Default constructor.
koilo::EulerOrder::EulerOrder()
    : AxisOrder(Axis::XYZ),
      FrameTaken(AxisFrame::Static),
      Permutation(0, 1, 2) {}

// Parameterized constructor.
koilo::EulerOrder::EulerOrder(Axis axisOrder, AxisFrame axisFrame, Vector3D permutation)
    : AxisOrder(axisOrder),
      FrameTaken(axisFrame),
      Permutation(permutation) {}

// Convert EulerOrder to a string representation.
koilo::UString koilo::EulerOrder::ToString() {
    return Permutation.ToString();
}

} // namespace koilo
