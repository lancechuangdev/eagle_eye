#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "settings_service.h"
#include "app_paths.h"
#include "file_utils.h"

nlohmann::json SettingsService::get_settings(const std::string &section_name)
{
    nlohmann::json settings;

    // Step 1: Read the JSON file
    std::ifstream settings_file(AppPaths::Settings_File_Path.string());
    if (!settings_file.is_open())
    {
        std::cerr << "Unable to open settings file: " << AppPaths::Settings_File_Path.string() << std::endl;
        return settings;
    }

    try
    {
        // Parse the JSON content
        nlohmann::json json_content;
        settings_file >> json_content;
        settings_file.close();

        // Check if the section exists
        if (json_content.contains(section_name))
        {
            // Extract the section as a JSON object
            settings = json_content[section_name];
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error reading JSON settings: " << e.what() << std::endl;
    }

    return settings;
}

void SettingsService::add_or_update_settings(const std::string &section_name, const nlohmann::json &new_settings) 
{
    auto settings_file_path = AppPaths::Settings_File_Path.string();

    // Ensure the settings file exists with empty JSON content if it doesn't exist
    if (!std::filesystem::exists(settings_file_path)) 
    {
        std::ofstream new_file(settings_file_path, std::ios::out | std::ios::trunc);
        if (new_file.is_open()) 
        {
            new_file << "{}";
            new_file.close();
        }
        else 
        {
            std::cerr << "Unable to create settings file: " << settings_file_path << std::endl;
            return;
        }
    }

    // Read the existing JSON content
    std::ifstream settings_file(settings_file_path);
    nlohmann::json json_data;
    if (settings_file.is_open()) 
    {
        try 
        {
            settings_file >> json_data;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error reading JSON from settings file: " << e.what() << std::endl;
            settings_file.close();
            return;
        }
        settings_file.close();
    } 
    else 
    {
        std::cerr << "Unable to open settings file: " << settings_file_path << std::endl;
        return;
    }

    // Update or create the specified section
    if (!json_data.contains(section_name)) 
    {
        json_data[section_name] = nlohmann::json::object();
    }

    for (const auto& [key, value] : new_settings.items()) 
    {
        json_data[section_name][key] = value;
    }

    // Write the updated JSON content back to the file
    std::ofstream out_file(settings_file_path, std::ios::out | std::ios::trunc);
    if (out_file.is_open()) 
    {
        try 
        {
            out_file << json_data.dump(4);
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error writing JSON to settings file: " << e.what() << std::endl;
        }
        out_file.close();
    } 
    else 
    {
        std::cerr << "Unable to open settings file for writing: " << settings_file_path << std::endl;
    }
}

void SettingsService::remove_settings(const std::string &section_name, const std::vector<std::string> &settings_to_remove)
{
    auto settings_file_path = AppPaths::Settings_File_Path.string();

    // Ensure the settings file exists with empty JSON content if it doesn't exist
    if (!std::filesystem::exists(settings_file_path)) 
    {
        std::ofstream new_file(settings_file_path, std::ios::out | std::ios::trunc);
        if (new_file.is_open()) 
        {
            new_file << "{}";
            new_file.close();
        }
        else 
        {
            std::cerr << "Unable to create settings file: " << settings_file_path << std::endl;
            return;
        }
    }

    // Read the existing JSON content
    std::ifstream settings_file(settings_file_path);
    nlohmann::json json_data;
    if (settings_file.is_open()) 
    {
        try 
        {
            settings_file >> json_data;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error reading JSON from settings file: " << e.what() << std::endl;
            settings_file.close();
            return;
        }
        settings_file.close();
    } 
    else 
    {
        std::cerr << "Unable to open settings file: " << settings_file_path << std::endl;
        return;
    }

    // remove the specified settings
    if (json_data.contains(section_name)) 
    {
        for (const auto& key: settings_to_remove) 
        {
            json_data[section_name].erase(key);
        }
    }

    // Write the updated JSON content back to the file
    std::ofstream out_file(settings_file_path, std::ios::out | std::ios::trunc);
    if (out_file.is_open()) 
    {
        try 
        {
            out_file << json_data.dump(4);
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error writing JSON to settings file: " << e.what() << std::endl;
        }
        out_file.close();
    } 
    else 
    {
        std::cerr << "Unable to open settings file for writing: " << settings_file_path << std::endl;
    }
}