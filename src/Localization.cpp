#include "Localization.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

Localization& Localization::get() {
    static Localization instance;
    return instance;
}

void Localization::loadLanguage(const std::string& langCode) {
    std::ifstream file("assets/lang/" + langCode + ".json");
    if (!file.is_open()) {
        std::cerr << "[i18n Error] Could not open language file: assets/lang/" << langCode << ".json\n";
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
        strings_.clear();
        for (auto& [key, value] : j.items()) {
            strings_[key] = value.get<std::string>();
        }
        currentLang_ = langCode;
    } catch (const std::exception& e) {
        std::cerr << "[i18n Error] Failed to parse language JSON: " << e.what() << '\n';
    }
}

const char* Localization::text(const std::string& key) const {
    auto it = strings_.find(key);
    if (it != strings_.end()) {
        return it->second.c_str();
    }
    return key.c_str(); // Fallback: Anahtar bulunamazsa anahtarın adını döndürür
}