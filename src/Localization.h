#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class Localization {
private:
    std::unordered_map<std::string, std::string> strings_;
    std::string currentLang_;
    std::vector<std::string> availableLangs_ = {"en", "tr"};

    // Singleton: Dışarıdan constructor çağrılamaz
    Localization() = default;

public:
    // Copy/Move constructor ve assignment operator'ları engelle
    Localization(const Localization&) = delete;
    Localization& operator=(const Localization&) = delete;

    static Localization& get();

    void loadLanguage(const std::string& langCode);
    const char* text(const std::string& key) const;

    const std::string& currentLang() const { return currentLang_; }
    const std::vector<std::string>& availableLangs() const { return availableLangs_; }
};