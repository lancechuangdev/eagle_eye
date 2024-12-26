#include "app_paths.h"

const std::filesystem::path AppPaths::Log_File_Path = AppPaths::getHomePath() / ".config" / "eagle_eye" / "app.log";
const std::filesystem::path AppPaths::Settings_File_Path = AppPaths::getHomePath() / ".config" / "eagle_eye" / "settings.ini";
const std::filesystem::path AppPaths::Detection_Results_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_results";
const std::filesystem::path AppPaths::Detection_Results_Archive_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_results_archive";
const std::filesystem::path AppPaths::Dataset_Path = AppPaths::getHomePath() / "eagle_eye" / "dataset";