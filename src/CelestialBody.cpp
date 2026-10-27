#include "CelestialBody.h"

CelestialBody::CelestialBody(std::string n, double m, Vec2 p, Vec2 v, double rKm,
                             std::array<std::uint8_t, 3> c)
    : name(std::move(n)), mass(m), pos(p), vel(v), radiusKm(rKm), color(c) {}

double CelestialBody::kineticEnergy() const { return 0.5 * mass * vel.norm2(); }

double CelestialBody::angularMomentumZ() const { return mass * pos.cross(vel); }

void CelestialBody::recordTrail(double minSpacing, std::size_t maxPoints) {
    if (trail.empty() || (pos - trail.back()).norm() >= minSpacing) {
        trail.push_back(pos);
        while (trail.size() > maxPoints) trail.pop_front();
    }
}
