#pragma once
#include <cmath>

// R^2 uzayinda 2D vektor (konum, hiz, ivme icin). double hassasiyet: astronomik olcekler.
struct Vec2 {
    double x{0.0}, y{0.0};

    constexpr Vec2() = default;
    constexpr Vec2(double x_, double y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(double s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(double s) const { return {x / s, y / s}; }
    constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }

    constexpr double dot(const Vec2& o) const { return x * o.x + y * o.y; }
    constexpr double cross(const Vec2& o) const { return x * o.y - y * o.x; } // z bileseni
    constexpr double norm2() const { return x * x + y * y; }
    double norm() const { return std::sqrt(norm2()); }
    Vec2 normalized() const { double n = norm(); return n > 0.0 ? *this / n : Vec2{}; }
};

constexpr Vec2 operator*(double s, const Vec2& v) { return v * s; }
