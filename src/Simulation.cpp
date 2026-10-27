#include "Simulation.h"
#include <algorithm>
#include <cmath>
#include <iomanip>

void Simulation::setBodies(std::vector<CelestialBody> bodies) {
    bodies_ = std::move(bodies);
    centerOnBarycenter();          // toplam momentum = 0, barycenter orijinde
    initial_ = bodies_;
    const std::size_t n = bodies_.size();
    a1_.assign(n, {}); a2_.assign(n, {}); posPred_.assign(n, {}); velPred_.assign(n, {});
    time_ = 0.0; stepCount_ = 0; accumulator_ = 0.0; backlog_ = false;
    E0_ = totalEnergy(); L0_ = totalAngularMomentum();
    resetHistory();
    if (!logPath_.empty()) openLog();
}

void Simulation::reset() {
    auto b = initial_;
    bodies_ = std::move(b);
    for (auto& body : bodies_) body.trail.clear();
    time_ = 0.0; stepCount_ = 0; accumulator_ = 0.0; backlog_ = false;
    E0_ = totalEnergy(); L0_ = totalAngularMomentum();
    resetHistory();
    if (!logPath_.empty()) openLog();
}

void Simulation::centerOnBarycenter() {
    double M = 0.0;
    Vec2 R, V;
    for (const auto& b : bodies_) { M += b.mass; R += b.pos * b.mass; V += b.vel * b.mass; }
    totalMass_ = M;
    R = R / M; V = V / M;
    for (auto& b : bodies_) { b.pos -= R; b.vel -= V; }
}

// a_i = sum_{j != i} G m_j (r_j - r_i) / |r_j - r_i|^3
void Simulation::computeAccelerations(const std::vector<Vec2>& pos, std::vector<Vec2>& acc) const {
    const std::size_t n = bodies_.size();
    for (std::size_t i = 0; i < n; ++i) acc[i] = {};
    // Newton'un 3. yasasi: her cifti bir kez hesapla, iki cisme de uygula
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            Vec2 d = pos[j] - pos[i];
            double r2 = d.norm2();
            double invR3 = 1.0 / (r2 * std::sqrt(r2));
            acc[i] += d * (G * bodies_[j].mass * invR3);
            acc[j] -= d * (G * bodies_[i].mass * invR3);
        }
    }
}

// Heun yontemi (RK2):
//   tahmin:  r* = r + v dt ;  v* = v + a(r) dt
//   duzeltme: v' = v + dt/2 (a(r) + a(r*)) ;  r' = r + dt/2 (v + v*)
void Simulation::step(double dt) {
    const std::size_t n = bodies_.size();
    for (std::size_t i = 0; i < n; ++i) posPred_[i] = bodies_[i].pos;
    computeAccelerations(posPred_, a1_);

    for (std::size_t i = 0; i < n; ++i) {
        velPred_[i] = bodies_[i].vel + a1_[i] * dt;
        posPred_[i] = bodies_[i].pos + bodies_[i].vel * dt;
    }
    computeAccelerations(posPred_, a2_);

    for (std::size_t i = 0; i < n; ++i) {
        auto& b = bodies_[i];
        Vec2 vOld = b.vel;
        b.vel = vOld + (a1_[i] + a2_[i]) * (0.5 * dt);
        b.pos = b.pos + (vOld + velPred_[i]) * (0.5 * dt);
    }

    time_ += dt;
    ++stepCount_;

    for (std::size_t i = 1; i < n; ++i) {   // i=0 Gunes: iz tutulmaz
        double spacing = settings.trailSpacingFactor * (bodies_[i].pos - bodies_[0].pos).norm();
        bodies_[i].recordTrail(spacing, settings.maxTrailPoints);
    }

    if (time_ >= nextSample_) {          // grafik gecmisi
        recordSample();
        nextSample_ += historyInterval_;
        if (nextSample_ <= time_) nextSample_ = time_ + historyInterval_;
    }

    if (log_.is_open() && time_ >= nextLog_) {
        writeLogRow();
        nextLog_ += logInterval_;
    }
}

// FIXED TIMESTEP + ACCUMULATOR: render FPS ne olursa olsun adim sayisi = simSure / dt.
int Simulation::update(double realDt) {
    if (settings.paused) return 0;
    realDt = std::min(realDt, 0.25);               // pencere surukleme/duraklama sicramalarini kes
    accumulator_ += realDt * settings.speed;

    int steps = 0;
    while (accumulator_ >= settings.dt && steps < settings.maxStepsPerFrame) {
        step(settings.dt);
        accumulator_ -= settings.dt;
        ++steps;
    }
    if (accumulator_ >= settings.dt) { backlog_ = true; accumulator_ = 0.0; } // yetisemiyoruz
    else backlog_ = false;
    return steps;
}

Vec2 Simulation::barycenter() const {
    Vec2 R;
    for (const auto& b : bodies_) R += b.pos * b.mass;
    return R / totalMass_;
}

void Simulation::resetHistory() {
    const std::size_t n = bodies_.size();
    history_.t.clear();
    history_.dist.assign(n, {});
    history_.speed.assign(n, {});
    history_.ke.assign(n, {});
    historyInterval_ = 86400.0;      // baslangic: 1 simule gun
    recordSample();                  // t = 0 ornegi
    nextSample_ = historyInterval_;
}

// Her cisim icin (barycenter uzakligi, hiz, kinetik enerji) ornegi alir.
// Ornek sayisi sinirini asinca her ikinci ornek silinir ve aralik ikiye katlanir:
// bellek sabit kalir, t=0'dan bugune tum gecmis korunur.
void Simulation::recordSample() {
    const Vec2 R = barycenter();
    history_.t.push_back(time_);
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        history_.dist[i].push_back((bodies_[i].pos - R).norm());
        history_.speed[i].push_back(bodies_[i].vel.norm());
        history_.ke[i].push_back(bodies_[i].kineticEnergy());
    }
    if (history_.t.size() > kMaxHistory) {
        auto halve = [](std::vector<double>& v) {
            std::size_t k = 0;
            for (std::size_t i = 0; i < v.size(); i += 2) v[k++] = v[i];
            v.resize(k);
        };
        halve(history_.t);
        for (std::size_t i = 0; i < bodies_.size(); ++i) {
            halve(history_.dist[i]); halve(history_.speed[i]); halve(history_.ke[i]);
        }
        historyInterval_ *= 2.0;
    }
}

double Simulation::totalEnergy() const {
    double ke = 0.0, pe = 0.0;
    const std::size_t n = bodies_.size();
    for (std::size_t i = 0; i < n; ++i) {
        ke += bodies_[i].kineticEnergy();
        for (std::size_t j = i + 1; j < n; ++j)
            pe -= G * bodies_[i].mass * bodies_[j].mass / (bodies_[j].pos - bodies_[i].pos).norm();
    }
    return ke + pe;
}

double Simulation::totalAngularMomentum() const {
    double L = 0.0;
    for (const auto& b : bodies_) L += b.angularMomentumZ();
    return L;
}

void Simulation::enableLogging(const std::string& path, double intervalSeconds) {
    logPath_ = path;
    logInterval_ = intervalSeconds;
    openLog();
}

void Simulation::openLog() {
    if (log_.is_open()) log_.close();
    log_.open(logPath_, std::ios::trunc);
    if (!log_) return;
    log_ << "# t[s] E_total[J] Lz[kg m^2/s]";
    for (const auto& b : bodies_)
        log_ << ' ' << b.name << "_x[m] " << b.name << "_y[m] "
             << b.name << "_vx[m/s] " << b.name << "_vy[m/s]";
    log_ << '\n';
    nextLog_ = 0.0;
    writeLogRow();
    nextLog_ = logInterval_;
}

void Simulation::writeLogRow() {
    log_ << std::scientific << std::setprecision(9)
         << time_ << ' ' << totalEnergy() << ' ' << totalAngularMomentum();
    for (const auto& b : bodies_)
        log_ << ' ' << b.pos.x << ' ' << b.pos.y << ' ' << b.vel.x << ' ' << b.vel.y;
    log_ << '\n';
}
