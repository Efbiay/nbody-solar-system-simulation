#pragma once
#include <string>
#include <vector>
#include "CelestialBody.h"

namespace DataIO {
// JSON icerikli (.txt uzantili) dosyadan cisimleri okur. Bkz. planets.txt
bool loadBodies(const std::string& path, std::vector<CelestialBody>& out, std::string& error);
}
