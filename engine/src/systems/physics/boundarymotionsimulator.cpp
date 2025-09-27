#include <ptx/systems/physics/boundarymotionsimulator.hpp>

BoundaryMotionSimulator::BoundaryMotionSimulator(Mesh** objects, int objectCount, Cube* bC, float radius, float elasticity) {
    this->bC = bC;
    this->sphereCount = objectCount;
    this->elasticity = elasticity;

    bS = new Sphere*[objectCount];
    for (int i = 0; i < objectCount; i++) {
        //bS[i] = new Sphere(objects[i], radius);
    }
}

void BoundaryMotionSimulator::Randomize(float range) {
    for (int i = 0; i < sphereCount; i++) {
        bS[i]->velocity.X = ptx::Random::Float(0.0f, 1.0f) * range - range / 2.0f;
        bS[i]->velocity.Y = ptx::Random::Float(0.0f, 1.0f) * range - range / 2.0f;
        bS[i]->velocity.Z = ptx::Random::Float(0.0f, 1.0f) * range - range / 2.0f;
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
        bS[i]->Update(dT, acceleration * RandomRatio(2.0f), rotation);

        for (int j = i + 1; j < sphereCount; j++) {
            bS[i]->Collide(elasticity, bS[j]);
        }

        //bC->CollideSphere(elasticity, bS[i]);
        //bS[i]->velocity = bS[i]->velocity * RandomRatio(0.0005f);
        //bS[i]->GetObject3D()->ResetVertices();
        //bS[i]->GetObject3D()->GetTransform()->SetPosition(bS[i]->position);
    }
}
