#ifndef EAGLE_EYE_FILE_UTILS_H
#define EAGLE_EYE_FILE_UTILS_H

#include <iostream>
#include <filesystem>
#include <random>
#include <gtkmm.h>
#include <sys/stat.h> // For mkdir()
#include <regex>
#include <giomm.h>

class FileUtils
{
public:
    static bool createSubdirectory(const std::string& parent, const std::string& sub);
    static bool createFile(const std::string& path);
    static std::string getGladeFilePath();
    static std::string getCssFilePath();
    static bool directoryExists(const std::string &parent, const std::string &sub);
};

#endif // EAGLE_EYE_FILE_UTILS_H