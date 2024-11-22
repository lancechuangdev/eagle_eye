#include <fstream>
#include <iostream>
#include "settings_service.h"
#include "app_paths.h"
#include "file_utils.h"
#include "web_socket_client.h"

std::map<std::string, std::string> SettingsService::get_settings(const std::string &settings_header)
{
    std::map<std::string, std::string> settings;

    // Step 1: Read the content of the settings file
    auto path = AppPaths::Settings_File_Path.string();
    std::ifstream settingsFile(path);
    if (!settingsFile.is_open())
    {
        std::cerr << "Unable to open settings file: " << AppPaths::Settings_File_Path.string() << std::endl;
        return settings;
    }

    std::stringstream buffer;
    buffer << settingsFile.rdbuf();
    settingsFile.close();
    std::string content = buffer.str();

    // Step 2: Find the requested section
    size_t sectionPos = content.find(settings_header);
    if (sectionPos == std::string::npos)
    {
        // Section not found
        return settings;
    }

    // Step 3: Extract the section content
    size_t nextSectionPos = content.find('[', sectionPos + 1); // Find the next section's starting position
    std::string sectionContent;
    if (nextSectionPos == std::string::npos)
    {
        // Section is the last one in the file
        sectionContent = content.substr(sectionPos + settings_header.length());
    }
    else
    {
        // Extract content up to the next section
        sectionContent = content.substr(sectionPos + settings_header.length(), nextSectionPos - sectionPos - settings_header.length());
    }

    // Step 4: Parse the key-value pairs
    std::istringstream sectionStream(sectionContent);
    std::string line;
    while (std::getline(sectionStream, line))
    {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Split the line into key and value
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos)
        {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            settings[key] = value;
        }
    }

    return settings;
}

void SettingsService::save_settings(std::string &settings_to_save, std::string &settings_header)
{
    if (!FileUtils::createFile(AppPaths::Settings_File_Path.string()))
    {
        return;
    }

    // Step 1: Read the existing content of the file
    std::ifstream settingsFile(AppPaths::Settings_File_Path.string());
    std::stringstream buffer;
    if (settingsFile.is_open())
    {
        buffer << settingsFile.rdbuf();
        settingsFile.close();
    }
    else
    {
        std::cerr << "Unable to open settings file: " << AppPaths::Settings_File_Path.string() << std::endl;
    }
    std::string content = buffer.str();

    // Step 2: Find if the section for the device already exists
    size_t sectionPos = content.find(settings_header);
    bool sectionExists = (sectionPos != std::string::npos);

    if (sectionExists)
    {
        // Step 3: If the section exists, replace its contents
        size_t nextSectionPos = content.find('[', sectionPos + 1); // Find the next section's starting position

        // Replace the old section with the new one
        if (nextSectionPos == std::string::npos)
        {
            // The section is the last one, so replace to the end of the file
            content.replace(sectionPos, std::string::npos, settings_to_save);
        }
        else
        {
            // Replace up to the next section
            content.replace(sectionPos, nextSectionPos - sectionPos, settings_to_save);
        }
    }
    else
    {
        // Step 4: If the section doesn't exist, append the new section at the end
        content += settings_to_save;
    }

    // Step 5: Write the updated content back to the file (overwrite)
    std::ofstream outFile(AppPaths::Settings_File_Path.string());
    if (outFile.is_open()) 
    {
        outFile << content;
        outFile.close();
    }
    else
    {
        std::cerr << "Unable to open settings file for writing." << std::endl;
    }
}