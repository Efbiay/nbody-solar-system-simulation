#include "DataIO.h"
#include <array>
#include <fstream>
#include <nlohmann/json.hpp>

namespace DataIO {

using nlohmann::json;

// planets.txt dosyasi .txt uzantili ama icerigi JSON'dur; ayristirmayi nlohmann/json yapar.
bool loadBodies(const std::string& path, std::vector<CelestialBody>& out, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "Could not open file: " + path; return false; }

    try {
        // allow_exceptions=true, ignore_comments=true
        const json root = json::parse(in, nullptr, true, true);

        out.clear();
        for (const auto& e : root.at("bodies")) {
            const auto& p = e.at("position");
            const auto& v = e.at("velocity");

            std::array<std::uint8_t, 3> color{255, 255, 255};
            if (e.contains("color")) {
                const auto& c = e.at("color");
                for (int k = 0; k < 3; ++k) color[k] = static_cast<std::uint8_t>(c.at(k).get<int>());
            }

            out.emplace_back(e.at("name").get<std::string>(),
                             e.at("mass").get<double>(),
                             Vec2{p.at(0).get<double>(), p.at(1).get<double>()},
                             Vec2{v.at(0).get<double>(), v.at(1).get<double>()},
                             e.value("radius_km", 1000.0),
                             color);
        }
    } catch (const json::exception& ex) {
        error = std::string("JSON error (") + path + "): " + ex.what();
        return false;
    }

    if (out.size() < 2) { error = "At least two bodies are required."; return false; }
    return true;
}

} // namespace DataIO
