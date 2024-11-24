#include <vector>
#include <iostream>
#include <algorithm> // For std::sort
#include "retention_manager.h"
#include "app_paths.h"
#include "settings_service.h"

// void RetentionManager::start_daily_limit_enforcer(std::chrono::minutes interval)
// {
//     m_stop_enforcer = false;

//     auto max_per_day = 1000;
//     auto settings = SettingsService::get_settings("[detection]");
//     for (const auto &[key, value] : settings)
//     {
//         if (key == "max_per_day")
//         {
//             max_per_day = std::stod(value);
//             break;
//         }
//     }

//     m_enforcer_thread = std::thread([this, max_per_day, interval]() {
//         std::unique_lock<std::mutex> lock(m_enforcer_mutex);
//         while (!m_stop_enforcer)
//         {
//             try
//             {
//                 enforce_daily_limit(AppPaths::Detection_Results_Path, max_per_day);
//                 std::cout << "Daily limit enforcement executed.\n";
//             }
//             catch (const std::exception& e)
//             {
//                 std::cerr << "Error during daily limit enforcement: " << e.what() << "\n";
//             }

//             // Wait for the interval or until stopped
//             if (m_stop_enforcer_cv.wait_for(lock, interval, [this] { return m_stop_enforcer.load(); }))
//             {
//                 break; // Exit the loop if stopped
//             }
//         }
//     });
// }

void RetentionManager::enforce_daily_limit()
{
    auto max_per_day = 1000;
    auto settings = SettingsService::get_settings("[detection]");
    for (const auto &[key, value] : settings)
    {
        if (key == "max_per_day")
        {
            max_per_day = std::stod(value);
            break;
        }
    }

    try
    {
        enforce_daily_limit(AppPaths::Detection_Results_Path, max_per_day);
        std::cout << "Daily limit enforcement executed.\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error during daily limit enforcement: " << e.what() << "\n";
    }
}

void RetentionManager::enforce_daily_limit(const std::filesystem::path& path, int max_per_day)
{
    std::vector<std::filesystem::directory_entry> directories;

    // Collect all directories
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory())
        {
            directories.push_back(entry);
        }
    }

    // Sort directories by modification time (newest first)
    std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b) {
        return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
    });

    // Enforce max_per_day limit in results path
    if (directories.size() > max_per_day)
    {
        for (size_t i = max_per_day; i < directories.size(); ++i)
        {
            std::filesystem::remove_all(directories[i].path());
        }
    }
}

// Stop the periodic enforcer
// void RetentionManager::stop_daily_limit_enforcer()
// {
//     {
//         // Notify the condition variable
//         std::lock_guard<std::mutex> lock(m_enforcer_mutex);
//         m_stop_enforcer = true;
//     }
//     m_stop_enforcer_cv.notify_all();
    
//     if (m_enforcer_thread.joinable())
//     {
//         m_enforcer_thread.join();
//     }
// }

void RetentionManager::enforce_archive_retention()
{
    auto days_to_retain = 30;
    auto settings = SettingsService::get_settings("[detection]");
    for (const auto &[key, value] : settings)
    {
        if (key == "days_to_retain")
        {
            days_to_retain = std::stod(value);
            break;
        }
    }

    // Enforce max_to_retain in archive path
    std::vector<std::filesystem::directory_entry> archive_folders;
    
    // Collect all directories in the archive path
    for (const auto &entry : std::filesystem::directory_iterator(AppPaths::Detection_Results_Archive_Path))
    {
        if (entry.is_directory())
        {
            archive_folders.push_back(entry);
        }
    }

    // If there are no folders or only one, no retention needed
    if (archive_folders.size() < 2)
        return;

    // Sort archive folders by modification time (newest first)
    std::sort(archive_folders.begin(), archive_folders.end(), [](const auto &a, const auto &b)
    {
        return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
    });

    // Get the newest folder's modification time
    auto newest_time = std::filesystem::last_write_time(archive_folders.front());

    // Calculate the oldest allowed time based on the max_to_retain days
    auto oldest_allowed_time = newest_time - std::chrono::hours(24 * days_to_retain);

    // Remove folders that are older than the oldest allowed time
    for (auto it = archive_folders.rbegin(); it != archive_folders.rend(); ++it)
    {
        if (std::filesystem::last_write_time(*it) < oldest_allowed_time)
        {
            std::filesystem::remove_all(it->path());  // Remove the folder
        }
        else
        {
            break;  // No need to check further once we find folders within the retention period
        }
    }
}

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

void RetentionManager::archive()
{
    // Get current time
    auto current_time = std::chrono::system_clock::now();

    // Iterate through the folders in the results path
    for (const auto &entry : std::filesystem::directory_iterator(AppPaths::Detection_Results_Path))
    {
        if (entry.is_directory())
        {
            // Get the last write time of the folder (std::filesystem::__file_clock::time_point)
            auto folder_time = std::filesystem::last_write_time(entry);

            // Convert filesystem file_time_type to std::time_t using the helper function
            auto folder_time_sys = to_time_t(folder_time);

            // Convert back to system_clock::time_point for further calculations
            auto folder_time_point = std::chrono::system_clock::from_time_t(folder_time_sys);

            // Calculate the duration difference in hours
            auto duration = std::chrono::duration_cast<std::chrono::hours>(current_time - folder_time_point);

            // If the folder is older than 24 hours, move it
            if (duration.count() > 24)
            {
                // Convert time_t to local time
                std::tm tm = *std::localtime(&folder_time_sys);

                // Format the date in mm-dd-yyyy
                std::ostringstream date_stream;
                date_stream << std::put_time(&tm, "%m-%d-%Y");
                std::string date_str = date_stream.str();

                // Create a folder for that date in the archive path
                std::filesystem::path date_folder = AppPaths::Detection_Results_Archive_Path / date_str;
                std::filesystem::create_directories(date_folder);

                // Move the folder to the date-specific folder in the archive path
                std::filesystem::rename(entry.path(), date_folder / entry.path().filename());
                std::cout << "Moved folder: " << entry.path() << " to " << date_folder / entry.path().filename() << std::endl;
            }
        }
    }
}