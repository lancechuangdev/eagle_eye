#ifndef EAGLE_EYE_FILE_UTILS_H
#define EAGLE_EYE_FILE_UTILS_H

#include <iostream>
#include <filesystem>
#include <random>
#include <gtkmm.h>
#include <sys/stat.h> // For mkdir()
#include <regex>
#include <giomm.h>
#include <optional>
#include <nlohmann/json.hpp>

class FileUtils
{
public:
    static bool createSubdirectory(const std::string& parent, const std::string& sub);
    static bool createFile(const std::string& path);
    static std::string getGladeFilePath();
    static std::string getCssFilePath();
    static bool directoryExists(const std::string &parent, const std::string &sub);
    static std::vector<std::filesystem::path> get_recent_folders(const std::filesystem::path &directory, size_t count);
    static std::vector<std::filesystem::path> get_folders_by_time(const std::filesystem::path &directory, const std::chrono::system_clock::time_point &start_time, const std::chrono::system_clock::time_point &end_time);
    static void delete_all_in_directory(std::filesystem::path dir_path);
    static std::optional<std::chrono::system_clock::time_point> get_creation_time(const std::string& folderPath);
    static std::optional<nlohmann::json> get_json(const std::string &folderPath);
    static std::optional<std::chrono::system_clock::time_point> get_transaction_time(const std::string &folderPath);
};

#endif // EAGLE_EYE_FILE_UTILS_H