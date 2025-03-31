#ifndef EAGLE_EYE_APP_PATHS_H
#define EAGLE_EYE_APP_PATHS_H

#include <filesystem>
#include <cstdlib>
#include <stdexcept>

class AppPaths
{
public:
    static const std::filesystem::path Log_File_Path;
    static const std::filesystem::path Settings_File_Path;
    static const std::filesystem::path Detection_Results_Path;
    static const std::filesystem::path Detection_Results_Archive_Path;
    static const std::filesystem::path Dataset_Path;
    static const std::filesystem::path Projects_Path;
    static std::filesystem::path Project_Detection_Results_Path(const std::string &project_name);
    static std::filesystem::path Project_Path(const std::string &project_name);
    static const std::string MODEL_PATH;
    static const std::filesystem::path MODEL_CACHE_PATH;
    
private:
    static std::filesystem::path getHomePath() {
        const char* home_env = std::getenv("HOME");
        if (!home_env) {
            throw std::runtime_error("HOME environment variable is not set");
        }
        return std::filesystem::path(home_env);
    }
};

#endif // APP_PATHS_H