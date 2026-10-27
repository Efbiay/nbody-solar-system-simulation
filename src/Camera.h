#pragma once
#include <SFML/System/Vector2.hpp>
#include "Vec2.h"

// Fiziksel (metre) -> ekran (piksel) dogrusal donusumu:
//   s = S * (p - f) + c ,  y ekseni ters cevrilir (ekranda y asagi artar)
class Camera {
public:
    double scale = 1e-9;      // piksel / metre
    Vec2 focus{0.0, 0.0};     // ekranin merkezinde gorunen dunya noktasi
    sf::Vector2f screenCenter{640.f, 400.f};

    sf::Vector2f worldToScreen(const Vec2& p) const {
        return {static_cast<float>(screenCenter.x + (p.x - focus.x) * scale),
                static_cast<float>(screenCenter.y - (p.y - focus.y) * scale)};
    }

    void zoom(double factor) { scale *= factor; }

    // radiusMeters yaricapli bir alan pencerenin kisa kenarina sigsin
    void fit(double radiusMeters, float w, float h) {
        scale = 0.45 * std::min(w, h) / radiusMeters;
    }
};
