// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/geometry/3d/sphere.hpp>
#include <cmath>


namespace koilo {

koilo::Sphere::Sphere(Vector3D position, float radius) {
    this->position = position;
    this->radius = radius;
}

float koilo::Sphere::GetRadius() {
    return radius;
}

void koilo::Sphere::Update(float dT, Vector3D acceleration, Quaternion rotation) {
    Quaternion rotationChange = rotation.Multiply(previousRotation.MultiplicativeInverse());
    velocity = rotationChange.RotateVector(velocity) * 0.999f + acceleration * dT;
    velocity = velocity.Constrain(-2500, 2500);
    position = position + velocity * dT;

    previousRotation = rotation;
}

bool koilo::Sphere::IsIntersecting(Sphere* bO) {
    return radius + bO->GetRadius() > fabs((position - bO->position).Magnitude());
}

void koilo::Sphere::Collide(float elasticity, Sphere* bO) {
    (void)elasticity;
    if (IsIntersecting(bO)) {
        Vector3D direction = (position - bO->position).Normal();
        Vector3D vDiff = velocity - bO->velocity;
        float fellingSpeed = vDiff.DotProduct(direction);

        if (fellingSpeed >= 0) {
            return;
        }

        float mass1 = 1.0f;
        float mass2 = 1.0f;

        float speed1 = (2 * mass2 * fellingSpeed) / (mass1 + mass2);
        float speed2 = (fellingSpeed * (mass2 - mass1)) / (mass1 + mass2);

        bO->velocity = bO->velocity + direction * speed1;
        this->velocity = this->velocity + direction * (speed2 - fellingSpeed);
    }
}

} // namespace koilo
