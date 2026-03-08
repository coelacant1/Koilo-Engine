// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <koilo/systems/scene/lighting/light.hpp>


namespace koilo {

koilo::Light::Light() {
    // Default constructor
}

koilo::Light::Light(Vector3D p, Vector3D intensity, float falloff, float a, float b)
    : p(p), intensity(intensity), falloff(falloff), a(a), b(b) {
}

void koilo::Light::Set(Vector3D p, Vector3D intensity, float falloff, float a, float b) {
    this->p = p;
    this->intensity = intensity;
    this->falloff = falloff;
    this->a = a;
    this->b = b;
}

void koilo::Light::SetIntensity(Vector3D intensity) {
    this->intensity = intensity;
}

void koilo::Light::SetFalloff(float falloff, float a, float b) {
    this->falloff = falloff;
    this->a = a;
    this->b = b;
}

void koilo::Light::MoveTo(Vector3D p) {
    this->p = p;
}

void koilo::Light::Translate(Vector3D p) {
    this->p = this->p + p;
}

void koilo::Light::SetFalloff(float falloff) {
    this->falloff = fabs(falloff);
}

void koilo::Light::SetCurve(float a, float b) {
    this->a = a;
    this->b = b;
}


Vector3D koilo::Light::GetPosition(){
    return p;
}

Vector3D koilo::Light::GetIntensity(){
    return intensity;
}

float koilo::Light::GetFalloff(){
    return falloff;
}

float koilo::Light::GetCurveA(){
    return a;
}

float koilo::Light::GetCurveB(){
    return b;
}

} // namespace koilo
