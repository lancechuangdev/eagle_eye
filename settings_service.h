#ifndef EAGLE_EYE_SETTINGS_SERVICE_H
#define EAGLE_EYE_SETTINGS_SERVICE_H

#include <string>
#include <map>
#include <nlohmann/json.hpp>

class SettingsService
{
public:
    static nlohmann::json get_settings(const std::string &section_name);
    static void add_or_update_settings(const std::string &section_name, const nlohmann::json &new_settings);
    static void remove_settings(const std::string &section_name, const std::vector<std::string> &settings_to_remove);
};

#endif