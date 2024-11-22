#ifndef EAGLE_EYE_SETTINGS_SERVICE_H
#define EAGLE_EYE_SETTINGS_SERVICE_H

#include <string>
#include <map>

class SettingsService
{
public:
    static std::map<std::string, std::string> get_settings(const std::string &settings_header);
    static void save_settings(std::string &settings_to_save, std::string &section_header);
};

#endif