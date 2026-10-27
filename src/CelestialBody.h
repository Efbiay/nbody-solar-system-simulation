#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include "Vec2.h"

// Gunes, gezegen veya uydu. Nokta kutle (point mass) varsayimi.
class CelestialBody {
public:
    std::string name;
    double mass{0.0};      // kg
    Vec2 pos;              // m  (barycenter-merkezli inertial cerceve)
    Vec2 vel;              // m/s
    double radiusKm{1.0};  // sadece gorsellestirme icin
    std::array<std::uint8_t, 3> color{255, 255, 255};

    std::deque<Vec2> trail; // yorunge izi (dunya koordinatlari)

    CelestialBody() = default;
    CelestialBody(std::string n, double m, Vec2 p, Vec2 v, double rKm,
                  std::array<std::uint8_t, 3> c);

    double kineticEnergy() const;     // 1/2 m v^2
    double angularMomentumZ() const;  // m (x vy - y vx), orijine gore

    // Son iz noktasindan en az minSpacing metre uzaklasildiysa yeni nokta ekler.
    void recordTrail(double minSpacing, std::size_t maxPoints);
};
