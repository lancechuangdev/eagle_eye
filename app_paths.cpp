#include "app_paths.h"

const std::filesystem::path AppPaths::Log_File_Path = AppPaths::getHomePath() / ".config" / "eagle_eye" / "app.log";
const std::filesystem::path AppPaths::Settings_File_Path = AppPaths::getHomePath() / ".config" / "eagle_eye" / "settings.ini";
// const std::filesystem::path AppPaths::Detection_Results_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_results";
// const std::filesystem::path AppPaths::Detection_Results_Archive_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_results_archive";
const std::filesystem::path AppPaths::Dataset_Path = AppPaths::getHomePath() / "eagle_eye" / "dataset";
const std::filesystem::path AppPaths::Projects_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_projects";
std::filesystem::path AppPaths::Project_Path(const std::string &project_name)
{
    return AppPaths::getHomePath() / "eagle_eye" / "detection_projects" / project_name;
}
std::filesystem::path AppPaths::Project_Detection_Results_Path(const std::string &project_name)
{
    return AppPaths::Project_Path(project_name) / "detection_results";
}
const std::string AppPaths::MODEL_PATH = "/usr/local/share/eagle_eye/model.onnx";
const std::filesystem::path AppPaths::MODEL_CACHE_PATH = AppPaths::getHomePath() / ".config" / "eagle_eye" / "engine_cache";