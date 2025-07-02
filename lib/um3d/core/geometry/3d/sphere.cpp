#include "sphere.hpp"

Sphere::Sphere(Vector3D position, float radius) {
    this->position = position;
    this->radius = radius;
}

float Sphere::GetRadius() {
    return radius;
}

void Sphere::Update(float dT, Vector3D acceleration, Quaternion rotation) {
    Quaternion rotationChange = rotation.Multiply(previousRotation.MultiplicativeInverse());
    velocity = rotationChange.RotateVector(velocity) * 0.999f + acceleration * dT; // maintain momentum of existing velocity, apply local acceleration
    velocity = velocity.Constrain(-2500, 2500);
    position = position + velocity * dT;

    previousRotation = rotation;
}

bool Sphere::IsIntersecting(Sphere* bO) {
    return radius + bO->GetRadius() > fabs((position - bO->position).Magnitude());
}

void Sphere::Collide(float elasticity, Sphere* bO) {
    if (IsIntersecting(bO)) {
        // collision
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
    } // else{//no collision}
}
