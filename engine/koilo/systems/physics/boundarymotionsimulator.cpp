#include <ptx/systems/physics/boundarymotionsimulator.hpp>

BoundaryMotionSimulator::BoundaryMotionSimulator(Mesh** objects, int objectCount, Cube* bC, float radius, float elasticity) {
    (void)objects;
    (void)radius;
    this->bC = bC;
    this->elasticity = elasticity;
    sphereCount = objectCount;

    spheres.reserve(objectCount);
    for (int i = 0; i < objectCount; i++) {
        spheres.push_back(nullptr); // Placeholder until Sphere construction is reinstated.
        // spheres[i] = new Sphere(objects[i], radius);
    }
}

void BoundaryMotionSimulator::Randomize(float range) {
    for (int i = 0; i < sphereCount; i++) {
        if (!spheres[i]) {
            continue;
        }

        spheres[i]->velocity.X = ptx::Random::Float(0.0f, 1.0f) * range - range / 2.0f;
        spheres[i]->velocity.Y = ptx::Random::Float(0.0f, 1.0f) * range - range / 2.0f;
        spheres[i]->velocity.Z = ptx::Random::Float(0.0f, 1.0f) * range - range / 2.0f;
    }
}

Vector3D BoundaryMotionSimulator::RandomRatio(float range) {
    Vector3D vRand;
    vRand.X = 1.0f + ptx::Random::Float(0.0f, 1.0f) * range;
    vRand.Y = 1.0f + ptx::Random::Float(0.0f, 1.0f) * range;
    vRand.Z = 1.0f + ptx::Random::Float(0.0f, 1.0f) * range;
    return vRand;
}

void BoundaryMotionSimulator::Update(float dT, Vector3D acceleration, Quaternion rotation) {
    for (int i = 0; i < sphereCount; i++) {
        if (!spheres[i]) {
            continue;
        }

        spheres[i]->Update(dT, acceleration * RandomRatio(2.0f), rotation);

        for (int j = i + 1; j < sphereCount; j++) {
            if (!spheres[j]) {
                continue;
            }
            spheres[i]->Collide(elasticity, spheres[j]);
        }

        //bC->CollideSphere(elasticity, spheres[i]);
        //spheres[i]->velocity = spheres[i]->velocity * RandomRatio(0.0005f);
        //spheres[i]->GetObject3D()->ResetVertices();
        //spheres[i]->GetObject3D()->GetTransform()->SetPosition(spheres[i]->position);
    }
}
