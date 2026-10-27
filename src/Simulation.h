#pragma once
#include <cmath>
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>
#include "CelestialBody.h"
#include "Constants.h"

// Fizik motoru. Render dongusunden TAMAMEN bagimsizdir:
// fizik her zaman sabit dt (fixed timestep) ile ilerler, kare hizi sonucu degistirmez.
class Simulation {
public:
    static constexpr double G = Const::G;
    static constexpr double AU = Const::AU;
    static constexpr double YEAR = Const::YEAR;

    struct Settings {
        double dt = 3600.0;                 // SABIT fizik adimi [s simule]
        double speed = 30.0 * 86400.0;      // simule saniye / gercek saniye
        bool paused = false;
        int maxStepsPerFrame = 20000;       // "spiral of death" korumasi
        double trailSpacingFactor = 0.02;   // Gunes'e uzakligin yuzdesi kadar hareketle iz noktasi
        std::size_t maxTrailPoints = 3000;
    } settings;

    void setBodies(std::vector<CelestialBody> bodies);
    void reset();

    // Gercek gecen sureyi biriktirir, sabit dt'lik adimlara boler. Atilan adim sayisini doner.
    int update(double realDtSeconds);

    // Tek bir Heun (Improved Euler / RK2) adimi.
    void step(double dt);

    void enableLogging(const std::string& path, double intervalSeconds);

    // --- Gecmis (t=0'dan bugune) : cisim basina grafikler icin ---
    struct History {
        std::vector<double> t;                             // [s]
        std::vector<std::vector<double>> dist;             // [cisim][ornek] barycenter uzakligi [m]
        std::vector<std::vector<double>> speed;            // [m/s]
        std::vector<std::vector<double>> ke;               // kinetik enerji [J]
    };
    const History& history() const { return history_; }
    Vec2 barycenter() const;

    // --- Gozlem ---
    const std::vector<CelestialBody>& bodies() const { return bodies_; }
    double time() const { return time_; }
    std::size_t stepCount() const { return stepCount_; }
    bool backlog() const { return backlog_; }
    double totalEnergy() const;
    double totalAngularMomentum() const;
    double energyDrift() const { return (totalEnergy() - E0_) / std::abs(E0_); }
    double angularMomentumDrift() const { return (totalAngularMomentum() - L0_) / std::abs(L0_); }

private:
    void centerOnBarycenter();
    void computeAccelerations(const std::vector<Vec2>& pos, std::vector<Vec2>& acc) const;
    void resetHistory();
    void recordSample();
    void writeLogRow();
    void openLog();

    std::vector<CelestialBody> bodies_, initial_;
    std::vector<Vec2> a1_, a2_, posPred_, velPred_;  // tekrar kullanilan tamponlar

    double accumulator_ = 0.0;
    double time_ = 0.0;
    std::size_t stepCount_ = 0;
    bool backlog_ = false;
    double E0_ = 1.0, L0_ = 1.0;

    static constexpr std::size_t kMaxHistory = 16384;  // asilinca ornekleme seyreltilir
    History history_;
    double historyInterval_ = 86400.0, nextSample_ = 0.0, totalMass_ = 0.0;

    std::ofstream log_;
    std::string logPath_;
    double logInterval_ = 0.0, nextLog_ = 0.0;
};
