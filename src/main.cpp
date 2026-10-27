#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include "Camera.h"
#include "DataIO.h"
#include "Localization.h"
#include "Plots.h"
#include "Simulation.h"

namespace {
    constexpr const char *kTitle = "Solar System N-Body (Heun RK2)";

    // ---------- Pencere ayarlari ----------
    const char *kResNames[] = {"1280 x 720", "1600 x 900", "1920 x 1080", "2560 x 1440"};
    const sf::Vector2u kRes[] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}};

    struct WindowConfig {
        int resIndex = 0; // uygulanmis cozunurluk
        bool fullscreen = false; // uygulanmis mod
        bool vsync = false;
        bool fpsUnlimited = false;
        int fpsLimit = 60;

        // "Uygula" ile gecerli olacak bekleyen degerler
        int pendingRes = 0;
        bool pendingFullscreen = false;
        bool recreate = false;
        bool maximized = false;
        sf::Vector2u restoreSize = kRes[0];
        sf::Vector2i restorePosition{};
    };

    void applyFramePacing(sf::RenderWindow &w, const WindowConfig &c) {
        w.setVerticalSyncEnabled(c.vsync);
        w.setFramerateLimit((c.vsync || c.fpsUnlimited) ? 0u : static_cast<unsigned>(c.fpsLimit));
    }

    void createWindow(sf::RenderWindow &w, const WindowConfig &c) {
        if (c.fullscreen)
            w.create(sf::VideoMode::getDesktopMode(), kTitle, sf::Style::None, sf::State::Fullscreen);
        else
            w.create(sf::VideoMode(kRes[c.resIndex]), kTitle, sf::Style::None, sf::State::Windowed);
        applyFramePacing(w, c); // create() bu ayarlari sifirlar, tekrar uygula
    }

    void toggleMaximize(sf::RenderWindow &window, WindowConfig &config) {
        if (config.fullscreen) {
            config.pendingFullscreen = false;
            config.recreate = true;
            return;
        }

        if (config.maximized) {
            window.setSize(config.restoreSize);
            window.setPosition(config.restorePosition);
            config.maximized = false;
            return;
        }

        config.restoreSize = window.getSize();
        config.restorePosition = window.getPosition();

        sf::Vector2i workAreaPosition{0, 0};
        sf::Vector2u workAreaSize = sf::VideoMode::getDesktopMode().size;
#ifdef _WIN32
        MONITORINFO monitorInfo{sizeof(MONITORINFO)};
        const auto monitor = MonitorFromWindow(window.getNativeHandle(), MONITOR_DEFAULTTONEAREST);
        if (GetMonitorInfo(monitor, &monitorInfo)) {
            workAreaPosition = {static_cast<int>(monitorInfo.rcWork.left),
                                static_cast<int>(monitorInfo.rcWork.top)};
            workAreaSize = {
                static_cast<unsigned>(monitorInfo.rcWork.right - monitorInfo.rcWork.left),
                static_cast<unsigned>(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top)
            };
        }
#endif
        window.setPosition(workAreaPosition);
        window.setSize(workAreaSize);
        config.maximized = true;
    }

    void minimizeWindow(sf::RenderWindow &window) {
#ifdef _WIN32
        ShowWindow(window.getNativeHandle(), SW_MINIMIZE);
#else
        (void)window;
#endif
    }

    float bodyDrawRadiusPx(const CelestialBody &b, float sizeMul) {
        // Logaritmik sikistirma: Gunes ile Merkur ayni ekranda okunabilir kalir
        float px = 3.0f + 1.8f * static_cast<float>(std::log10(std::max(b.radiusKm, 1000.0) / 1000.0));
        return px * sizeMul;
    }
} // namespace

int main() {
    std::vector<CelestialBody> bodies;
    std::string err;
    if (!DataIO::loadBodies("assets/planets.txt", bodies, err)) {
        std::cerr << "Error: " << err << '\n';
        return 1;
    }

    Simulation sim;
    sim.setBodies(std::move(bodies));
    sim.enableLogging("output.txt", 86400.0); // 1 gunde bir satir

    WindowConfig wc;
    sf::RenderWindow window;
    createWindow(window, wc);
    if (!ImGui::SFML::Init(window)) return 1;

    Camera cam;
    cam.screenCenter = sf::Vector2f(window.getSize()) / 2.f;
    cam.fit(32.0 * Simulation::AU, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));

    int tracked = -1; // -1 = barycenter
    bool showTrails = true;
    float sizeMul = 1.0f;
    float speedDays = 30.0f; // simule gun / gercek saniye
    float dtSeconds = 3600.f;
    int stepsThisFrame = 0;
    float stepsPerSec = 0.f;
    std::vector<bool> plotOpen(sim.bodies().size(), false); // cisim basina grafik paneli
    int focusPlot = -1;

    sf::Clock deltaClock;
    std::vector<sf::Vertex> verts;

    sf::Texture uniLogo;
    uniLogo.loadFromFile("assets/uni_logo.png"); // Üniversitenin logosu

    sf::Texture claudeLogo;
    claudeLogo.loadFromFile("assets/claude_logo.png"); // Claude logosu

    Localization::get().loadLanguage("en");

    bool showAboutWindow = false; // Popup penceresinin açık/kapalı durumu
    bool draggingTitleBar = false;
    sf::Vector2i lastDragMousePosition{};

    while (window.isOpen()) {
        // ----- Pencereyi yeniden olustur (ImGui karesi disinda) -----
        if (wc.recreate) {
            wc.recreate = false;
            wc.resIndex = wc.pendingRes;
            wc.fullscreen = wc.pendingFullscreen;
            wc.maximized = false;
            ImGui::SFML::Shutdown(window);
            createWindow(window, wc);
            if (!ImGui::SFML::Init(window)) return 1;
            cam.screenCenter = sf::Vector2f(window.getSize()) / 2.f;
            deltaClock.restart();
        }

        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto *rs = event->getIf<sf::Event::Resized>()) {
                const sf::Vector2f sz(static_cast<float>(rs->size.x), static_cast<float>(rs->size.y));
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, sz)));
                cam.screenCenter = sz / 2.f;
            } else if (const auto *wh = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (!ImGui::GetIO().WantCaptureMouse) cam.zoom(std::pow(1.1, wh->delta));
            } else if (const auto *mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                // Cisme tiklama: en yakin cismi bul, grafik panelini ac
                if (mb->button == sf::Mouse::Button::Left && mb->position.y > 32 &&
                    !ImGui::GetIO().WantCaptureMouse) {
                    const sf::Vector2f m(mb->position);
                    int best = -1;
                    float bestD2 = 1e30f;
                    for (std::size_t i = 0; i < sim.bodies().size(); ++i) {
                        const sf::Vector2f sp = cam.worldToScreen(sim.bodies()[i].pos);
                        const float hit = std::max(bodyDrawRadiusPx(sim.bodies()[i], sizeMul) + 6.f, 12.f);
                        const float d2 = (sp.x - m.x) * (sp.x - m.x) + (sp.y - m.y) * (sp.y - m.y);
                        if (d2 <= hit * hit && d2 < bestD2) {
                            bestD2 = d2;
                            best = static_cast<int>(i);
                        }
                    }
                    if (best >= 0) {
                        plotOpen[best] = true;
                        focusPlot = best;
                    }
                }
            } else if (event->is<sf::Event::MouseButtonReleased>()) {
                draggingTitleBar = false;
            } else if (const auto *kp = event->getIf<sf::Event::KeyPressed>()) {
                if (!ImGui::GetIO().WantCaptureKeyboard) {
                    if (kp->code == sf::Keyboard::Key::Space) sim.settings.paused = !sim.settings.paused;
                    if (kp->code == sf::Keyboard::Key::F11) {
                        wc.pendingFullscreen = !wc.fullscreen;
                        wc.recreate = true;
                    }
                }
            }
        }

        const sf::Time frameDt = deltaClock.restart();
        ImGui::SFML::Update(window, frameDt);


        // Ana oyun döngüsünün (while) içinde, ImGui::Update sonrasına ekle:

        if (ImGui::BeginMainMenuBar()) {
            ImGui::TextUnformatted(kTitle);
            ImGui::SameLine();

            // DİL SEÇİM MENÜSÜ
            if (ImGui::BeginMenu(Localization::get().text("menu_language"))) {
                for (const auto& lang : Localization::get().availableLangs()) {
                    // Aktif dilin yaninda tik isareti gosterir
                    bool isSelected = (Localization::get().currentLang() == lang);
                    const char *languageName = Localization::get().text(lang == "en" ? "language_english" : "language_turkish");
                    if (ImGui::MenuItem(languageName, nullptr, isSelected)) {
                        Localization::get().loadLanguage(lang);
                    }
                }
                ImGui::EndMenu();
            }



            if (ImGui::BeginMenu(Localization::get().text("menu_help"))) {
                if (ImGui::MenuItem(Localization::get().text("help_docs"))) {
                    // İşletim sistemine göre dosyayı varsayılan okuyucu ile açar
#ifdef _WIN32
                    system("start README.md"); // Windows için
#elif __APPLE__
                    system("open README.md"); // MacOS için
#else
                    system("xdg-open README.md"); // Linux için
#endif
                }

                if (ImGui::MenuItem(Localization::get().text("help_about"))) {
                    showAboutWindow = true; // Popup penceresini tetikler
                }

                ImGui::EndMenu();
            }

            const float controlWidth = 3.f * 40.f + 2.f * ImGui::GetStyle().ItemSpacing.x;
            const float controlsStart = ImGui::GetWindowWidth() - controlWidth - ImGui::GetStyle().WindowPadding.x;
            ImGui::SameLine();
            const float dragWidth = controlsStart - ImGui::GetCursorPosX();
            if (dragWidth > 0.f) {
                ImGui::InvisibleButton("##title_bar_drag", {dragWidth, ImGui::GetFrameHeight()});
                if (ImGui::IsItemActive()) {
                    if (!draggingTitleBar) {
                        draggingTitleBar = true;
                        lastDragMousePosition = sf::Mouse::getPosition();
                    } else if (!wc.fullscreen && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        const sf::Vector2i mousePosition = sf::Mouse::getPosition();
                        const sf::Vector2i delta = mousePosition - lastDragMousePosition;
                        if (wc.maximized) {
                            toggleMaximize(window, wc);
                            window.setPosition({mousePosition.x - static_cast<int>(window.getSize().x / 2),
                                                mousePosition.y - static_cast<int>(ImGui::GetFrameHeight() / 2.f)});
                        } else {
                            window.setPosition(window.getPosition() + delta);
                        }
                        lastDragMousePosition = mousePosition;
                    }
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        toggleMaximize(window, wc);
                } else {
                    draggingTitleBar = false;
                }
            }

            ImGui::SameLine();
            ImGui::SetCursorPosX(controlsStart);
#ifdef _WIN32
            if (ImGui::Button("-", {40.f, 0.f}))
                minimizeWindow(window);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::get().text("window_minimize"));
#else
            ImGui::BeginDisabled();
            ImGui::Button("-", {40.f, 0.f});
            ImGui::EndDisabled();
#endif
            ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::Button(wc.maximized ? "[]" : "[ ]", {40.f, 0.f}))
                toggleMaximize(window, wc);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::get().text(wc.maximized ? "window_restore" : "window_maximize"));
            ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.12f, 0.12f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.18f, 0.18f, 1.f));
            if (ImGui::Button("X", {40.f, 0.f}))
                window.close();
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::get().text("window_close"));

            ImGui::EndMainMenuBar();
        }
        if (showAboutWindow) {
            // Pencereyi otomatik boyutlanacak şekilde aç
            ImGui::Begin(Localization::get().text("about_title"), &showAboutWindow, ImGuiWindowFlags_AlwaysAutoResize);

            // 1. Üniversite ve Öğrenci Bilgileri
            // Resmi 100x100 piksel boyutunda çizer (kendi logona göre ayarlayabilirsin)
            ImGui::Image(uniLogo, sf::Vector2f(200.f, 100.f));
            ImGui::SameLine();

            // Metni resmin yanına dikey olarak hizalamak için boşluk bırakıyoruz
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.f);
            ImGui::Text("%s: Mustafa Ahmet Koruç\n%s: Sivas Bilim ve Teknoloji Üniversitesi\n%s: Elektrik-Elektronik Muhendisligi",
                        Localization::get().text("about_name"),
                        Localization::get().text("about_university"),
                        Localization::get().text("about_department"));

            ImGui::Separator();

            // 2. Yapay Zeka Atıfı

            ImGui::Image(claudeLogo, sf::Vector2f(100.f, 100.f));
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.f);
            ImGui::TextWrapped("%s", Localization::get().text("about_ai_credit"));

            ImGui::End();
        }

        // ===== GAME TICK: fizik, kare hizindan bagimsiz sabit dt ile ilerler =====
        sim.settings.speed = static_cast<double>(speedDays) * 86400.0;
        sim.settings.dt = static_cast<double>(dtSeconds);
        stepsThisFrame = sim.update(frameDt.asSeconds());
        if (frameDt.asSeconds() > 0.f)
            stepsPerSec = 0.9f * stepsPerSec + 0.1f * (stepsThisFrame / frameDt.asSeconds());

        cam.focus = (tracked >= 0) ? sim.bodies()[tracked].pos : Vec2{0.0, 0.0};
        const auto winSize = window.getSize();

        // ===== ImGui paneli =====
        ImGui::SetNextWindowPos({10, 40}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({360, 0}, ImGuiCond_FirstUseEver);
        ImGui::Begin(Localization::get().text("sim_panel_title"));

        // ---------- Simulasyon ----------
        if (ImGui::CollapsingHeader(Localization::get().text("section_simulation"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button(Localization::get().text(sim.settings.paused ? "btn_resume" : "btn_pause")))
                sim.settings.paused = !sim.settings.paused;
            ImGui::SameLine();
            ImGui::BeginDisabled(!sim.settings.paused);
            if (ImGui::Button(Localization::get().text("btn_single_step"))) sim.step(sim.settings.dt);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button(Localization::get().text("btn_reset"))) sim.reset();

            ImGui::TextDisabled("%s", Localization::get().text("speed_presets"));
            struct Preset {
                const char *key;
                float days;
            };
            static const Preset presets[] = {
                {"preset_real_time", 1.f / 86400.f}, {"preset_day", 1.f},
                {"preset_week", 7.f}, {"preset_month", 30.44f},
                {"preset_year", 365.25f}, {"preset_ten_years", 3652.5f}
            };
            for (int i = 0; i < 6; ++i) {
                if (i % 3 != 0) ImGui::SameLine();
                if (ImGui::Button(Localization::get().text(presets[i].key))) speedDays = presets[i].days;
            }
            ImGui::SliderFloat(Localization::get().text("speed_days_per_second"), &speedDays, 1e-5f, 36525.f, "%.5g", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat(Localization::get().text("timestep_seconds"), &dtSeconds, 1.f, 86400.f, "%.0f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderInt(Localization::get().text("max_steps_per_frame"), &sim.settings.maxStepsPerFrame, 100, 100000, "%d",
                             ImGuiSliderFlags_Logarithmic);

            const double needed = sim.settings.speed / sim.settings.dt;
            ImGui::Text(Localization::get().text("required_steps_per_second"), needed);

            int trailLen = static_cast<int>(sim.settings.maxTrailPoints);
            if (ImGui::SliderInt(Localization::get().text("trail_length"), &trailLen, 100, 20000))
                sim.settings.maxTrailPoints = static_cast<std::size_t>(trailLen);
            float spacing = static_cast<float>(sim.settings.trailSpacingFactor);
            if (ImGui::SliderFloat(Localization::get().text("trail_spacing"), &spacing, 0.002f, 0.1f, "%.3f"))
                sim.settings.trailSpacingFactor = spacing;
        }

        // ---------- Gorunum ----------
        if (ImGui::CollapsingHeader(Localization::get().text("section_view"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox(Localization::get().text("show_trails"), &showTrails);
            ImGui::TextDisabled("%s", Localization::get().text("click_body_hint"));
            ImGui::SliderFloat(Localization::get().text("body_size"), &sizeMul, 0.5f, 3.f);
            if (ImGui::Button(Localization::get().text("inner_planets")))
                cam.fit(2.0 * Simulation::AU, static_cast<float>(winSize.x), static_cast<float>(winSize.y));
            ImGui::SameLine();
            if (ImGui::Button(Localization::get().text("whole_system")))
                cam.fit(32.0 * Simulation::AU, static_cast<float>(winSize.x), static_cast<float>(winSize.y));

            const char *preview = tracked < 0 ? Localization::get().text("barycenter") : sim.bodies()[tracked].name.c_str();
            if (ImGui::BeginCombo(Localization::get().text("tracking"), preview)) {
                if (ImGui::Selectable(Localization::get().text("barycenter"), tracked < 0)) tracked = -1;
                for (int i = 0; i < static_cast<int>(sim.bodies().size()); ++i)
                    if (ImGui::Selectable(sim.bodies()[i].name.c_str(), tracked == i)) tracked = i;
                ImGui::EndCombo();
            }
        }

        // ---------- Pencere ----------
        if (ImGui::CollapsingHeader(Localization::get().text("section_window"))) {
            if (ImGui::Checkbox(Localization::get().text("vsync"), &wc.vsync)) applyFramePacing(window, wc);

            ImGui::BeginDisabled(wc.vsync);
            if (ImGui::Checkbox(Localization::get().text("unlimited_fps"), &wc.fpsUnlimited)) applyFramePacing(window, wc);
            ImGui::BeginDisabled(wc.fpsUnlimited);
            if (ImGui::SliderInt(Localization::get().text("fps_limit"), &wc.fpsLimit, 15, 360)) applyFramePacing(window, wc);
            ImGui::EndDisabled();
            ImGui::EndDisabled();
            ImGui::TextDisabled("%s", Localization::get().text("fps_limit_hint"));

            ImGui::Separator();
            ImGui::Checkbox(Localization::get().text("fullscreen"), &wc.pendingFullscreen);
            ImGui::BeginDisabled(wc.pendingFullscreen);
            ImGui::Combo(Localization::get().text("resolution"), &wc.pendingRes, kResNames, 4);
            ImGui::EndDisabled();
            if (ImGui::Button(Localization::get().text("apply"))) wc.recreate = true;
            ImGui::SameLine();
            ImGui::Text(Localization::get().text("current_resolution"), winSize.x, winSize.y);
        }

        // ---------- Istatistik ----------
        if (ImGui::CollapsingHeader(Localization::get().text("section_statistics"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text(Localization::get().text("stat_sim_time"), sim.time() / Simulation::YEAR);
            ImGui::Text(Localization::get().text("stat_total_steps"), sim.stepCount());
            ImGui::Text(Localization::get().text("stat_steps_per_second"), stepsPerSec, stepsThisFrame);
            ImGui::Text(Localization::get().text("stat_energy_drift"), sim.energyDrift());
            ImGui::Text(Localization::get().text("stat_angular_momentum_drift"), sim.angularMomentumDrift());
            ImGui::Text(Localization::get().text("stat_fps"), ImGui::GetIO().Framerate);
            if (sim.backlog())
                ImGui::TextColored({1.f, 0.4f, 0.2f, 1.f}, "%s", Localization::get().text("warning_simulation_backlog"));
        }
        ImGui::End();

        // ===== Cisim grafik panelleri (t=0 -> simdiki simulasyon zamani) =====
        {
            const auto &hist = sim.history();
            const Vec2 bary = sim.barycenter();
            int cascade = 0;
            for (std::size_t i = 0; i < sim.bodies().size(); ++i) {
                if (!plotOpen[i]) continue;
                const CelestialBody &b = sim.bodies()[i];
                const ImU32 col = IM_COL32(b.color[0], b.color[1], b.color[2], 255);

                ImGui::SetNextWindowPos({400.f + 28.f * cascade, 45.f + 28.f * cascade}, ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize({480.f, 580.f}, ImGuiCond_FirstUseEver);
                if (focusPlot == static_cast<int>(i)) {
                    ImGui::SetNextWindowFocus();
                    focusPlot = -1;
                }

                bool open = true;
                const std::string title = std::string(Localization::get().text("plot_title")) + ": " + b.name + "###plot" + std::to_string(i);
                if (ImGui::Begin(title.c_str(), &open)) {
                    const double dNow = (b.pos - bary).norm();
                    const double vNow = b.vel.norm();
                    const double kNow = b.kineticEnergy();
                    ImGui::Text(Localization::get().text("body_mass"), b.mass);
                    ImGui::Text(Localization::get().text("body_distance_speed"), dNow / Simulation::AU, vNow / 1000.0);
                    ImGui::Text(Localization::get().text("body_kinetic_energy"), kNow);
                    ImGui::Separator();

                    Plots::line("##dist", Localization::get().text("plot_barycenter_distance"), "AU",
                                hist.t, hist.dist[i], sim.time(), dNow,
                                1.0 / Simulation::YEAR, 1.0 / Simulation::AU, 150.f, col);
                    ImGui::Spacing();
                    Plots::line("##speed", Localization::get().text("plot_speed"), "km/s",
                                hist.t, hist.speed[i], sim.time(), vNow,
                                1.0 / Simulation::YEAR, 1e-3, 150.f, col);
                    ImGui::Spacing();
                    Plots::line("##ke", Localization::get().text("plot_kinetic_energy"), "J",
                                hist.t, hist.ke[i], sim.time(), kNow,
                                1.0 / Simulation::YEAR, 1.0, 150.f, col);
                }
                ImGui::End();
                plotOpen[i] = open;
                ++cascade;
            }
        }

        // ===== Cizim =====
        window.clear(sf::Color(5, 5, 12));

        for (std::size_t bi = 0; bi < sim.bodies().size(); ++bi) {
            const auto &b = sim.bodies()[bi];
            const sf::Color col(b.color[0], b.color[1], b.color[2]);
            if (showTrails && b.trail.size() > 1) {
                verts.clear();
                const sf::Color tc(b.color[0], b.color[1], b.color[2], 130);
                for (const Vec2 &p: b.trail) verts.push_back(sf::Vertex{cam.worldToScreen(p), tc});
                verts.push_back(sf::Vertex{cam.worldToScreen(b.pos), tc});
                window.draw(verts.data(), verts.size(), sf::PrimitiveType::LineStrip);
            }
            const float r = bodyDrawRadiusPx(b, sizeMul);
            sf::CircleShape shape(r);
            shape.setOrigin({r, r});
            shape.setPosition(cam.worldToScreen(b.pos));
            shape.setFillColor(col);
            shape.setOutlineThickness(plotOpen[bi] ? 2.f : 0.f); // grafigi acik cisim isaretlenir
            shape.setOutlineColor(sf::Color::White);
            window.draw(shape);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
