#include <sys/syscall.h> // For syscall() and SYS_statx
#include <fcntl.h>      // For AT_FDCWD and AT_NO_AUTOMOUNT
#include <fstream>  // For std::ifstream
#include "file_utils.h"

std::string FileUtils::getGladeFilePath()
{
    const std::filesystem::path dev_path = "../ui.glade";
    const std::filesystem::path install_path = "/usr/local/share/eagle_eye/ui.glade";

    if (std::filesystem::exists(dev_path))
    {
        return dev_path;
    }
    else if (std::filesystem::exists(install_path))
    {
        return install_path;
    }
    else
    {
        std::cerr << "UI file not found!" << std::endl;
        return "";
    }
}

std::string FileUtils::getCssFilePath()
{
    const std::filesystem::path dev_path = "../style.css";
    const std::filesystem::path install_path = "/usr/local/share/eagle_eye/style.css";

    if (std::filesystem::exists(dev_path))
    {
        return dev_path;
    }
    else if (std::filesystem::exists(install_path))
    {
        return install_path;
    }
    else
    {
        std::cerr << "CSS file not found!" << std::endl;
        return "";
    }
}

bool FileUtils::createSubdirectory(const std::string &parent, const std::string &sub)
{  
    // Check if the parent directory exists; if not, create it recursively
    struct stat st;
    if (stat(parent.c_str(), &st) != 0)
    {
        // Parent directory does not exist, create it
        if (mkdir(parent.c_str(), 0755) != 0)
        {
            std::cerr << "Error creating parent directory: " << strerror(errno) << std::endl;
            return false; // Failure to create the parent directory
        }
    }

    // Check if the directory already exists
    if (directoryExists(parent, sub))
    {
        // If the directory exists, do nothing
        return true;
    }

    // Build the full path
    std::string path = Glib::build_filename(parent, sub);

    // Create the sub-directory (if it doesn't exist)
    if (mkdir(path.c_str(), 0755) == 0)
    {
        // Directory created successfully
        return true;
    }
    else
    {
        // Check if the failure was because the directory already exists
        if (errno == EEXIST)
        {
            return true; // Directory exists
        }
        else
        {
            // Some other error occurred
            std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
            return false;
        }
    }
}

bool FileUtils::createFile(const std::string& path)
{
    try
    {
        // Create the file object
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
        
        // Check if the file already exists
        if (file->query_exists())
        {
            std::cout << "File already exists: " << path << std::endl;
            return true;
        }

        // Get the parent directory of the file
        Glib::RefPtr<Gio::File> parentDir = file->get_parent();
        
        // Check if the parent directory exists
        if (!parentDir->query_exists())
        {
            // Create the directory and any missing parent directories
            parentDir->make_directory_with_parents();
        }

        // Now create the file
        Glib::RefPtr<Gio::FileOutputStream> outputStream = file->create_file();
        if (outputStream)
        {
            std::cout << "File created: " << path << std::endl;
            return true;
        }
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "Error creating file: " << ex.what() << std::endl;
    }

    return false;
}

bool FileUtils::directoryExists(const std::string &parent, const std::string &sub)
{
    // Build the full path
    std::string path = Glib::build_filename(parent, sub);

    // Check if the directory already exists
    return Glib::file_test(path, Glib::FILE_TEST_IS_DIR);
}

std::vector<std::filesystem::path> FileUtils::get_folders_by_time(const std::filesystem::path &directory, const std::chrono::system_clock::time_point &start_time, const std::chrono::system_clock::time_point &end_time)
{
    std::vector<std::filesystem::path> matching_folders;

    try
    {
        // Collect folders, sort them by creation time
        std::vector<std::filesystem::directory_entry> directories;
        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_directory())
            {
                directories.push_back(entry);
            }
        }

        std::sort(directories.begin(), directories.end(), [](const std::filesystem::directory_entry &a, const std::filesystem::directory_entry &b)
        {
            auto creation_time_a = FileUtils::get_transaction_time(a.path().string());
            auto creation_time_b = FileUtils::get_transaction_time(b.path().string());

            // If creation time is unavailable, treat it as later than any valid time
            if (!creation_time_a) return false;
            if (!creation_time_b) return true;

            return *creation_time_a < *creation_time_b;
        });

        // Iterate through sorted directories and filter by creation time
        for (const auto &entry : directories)
        {
            auto creation_time = FileUtils::get_transaction_time(entry.path().string());

            if (!creation_time)
            {
                std::cerr << "Failed to retrieve creation time for: " << entry.path() << std::endl;
                continue;
            }

            if (*creation_time > end_time)
            {
                // Early quit: subsequent folders will also be out of range
                break;
            }

            if (*creation_time >= start_time && *creation_time <= end_time)
            {
                matching_folders.push_back(entry.path());
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return matching_folders;
}

std::vector<std::filesystem::path> FileUtils::get_recent_folders(const std::filesystem::path& directory, size_t count)
{
    std::vector<std::filesystem::path> folders;

    // Iterate through the directory and collect only folders
    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (std::filesystem::is_directory(entry.status()))
        {
            folders.push_back(entry.path());
        }
    }

    // Sort the folders by last write time (modification time)
    std::sort(folders.begin(), folders.end(), [](const std::filesystem::path &a, const std::filesystem::path &b)
    {
        return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b); // Descending order
    });

    // Return the last 'count' folders (or fewer if there aren't enough)
    if (folders.size() > count)
    {
        folders.resize(count);
    }

    return folders;
}

void FileUtils::delete_all_in_directory(std::filesystem::path dir_path)
{
    try
    {
        // Check if the directory exists
        if (std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path))
        {
            for (const auto &entry : std::filesystem::directory_iterator(dir_path))
            {
                std::filesystem::remove_all(entry); // Remove both files and directories recursively
            }
            std::cout << "All contents in \"" << dir_path.string() << "\" have been deleted." << std::endl;
        }
        else
        {
            std::cerr << "The path \"" << dir_path.string() << "\" is not a valid directory." << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General error: " << e.what() << std::endl;
    }
}

std::optional<std::chrono::system_clock::time_point> FileUtils::get_creation_time(const std::string &folderPath)
{
    struct statx statxBuf;
    int result = syscall(SYS_statx, AT_FDCWD, folderPath.c_str(), AT_NO_AUTOMOUNT, STATX_BTIME, &statxBuf);

    if (result == 0 && (statxBuf.stx_mask & STATX_BTIME))
    {
        // Convert the `stx_btime` to std::chrono::system_clock::time_point
        auto btime = std::chrono::system_clock::from_time_t(statxBuf.stx_btime.tv_sec);
        btime += std::chrono::nanoseconds(statxBuf.stx_btime.tv_nsec);
        return btime;
    }
    else
    {
        std::cerr << "Creation time not available or syscall failed on: " << folderPath << strerror(errno) << std::endl;
        return std::nullopt;
    }
}

std::optional<nlohmann::json> FileUtils::get_json(const std::string &folderPath)
{
    std::filesystem::path trans_json_path = std::filesystem::path(folderPath) / "transaction_data.json";
    if (!std::filesystem::exists(trans_json_path))
    {
        std::cerr << "File not exists: transaction_data.json" << std::endl;
        return std::nullopt;
    }

    // Read the content of the JSON file
    std::ifstream json_file(trans_json_path);
    if (!json_file.is_open())
    {
        std::cerr << "Failed to open the file." << std::endl;
        return std::nullopt;
    }

    try
    {
        nlohmann::json json_data;
        json_file >> json_data;
        json_file.close();

        return json_data;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return std::nullopt;
    }    
}

std::optional<std::chrono::system_clock::time_point> FileUtils::get_transaction_time(const std::string &folderPath)
{
    try
    {
        auto json_data = FileUtils::get_json(folderPath);
        if (!json_data.has_value())
        {
            throw std::runtime_error("Failed to get json from the folder path");
        }

        const auto &json = json_data.value();
        if (!json.contains("transaction_datetime"))
        {
            throw std::runtime_error("Failed to get 'transaction_datetime' from json");
        }
        std::string datetime_str = json["transaction_datetime"];

        // Step 1: Parse the date and time part into a std::tm structure
        std::tm tm = {};
        std::istringstream ss(datetime_str.substr(0, 19)); // Exclude milliseconds for now
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail()) {
            throw std::runtime_error("Failed to parse datetime string");
        }

        // Step 2: Convert std::tm to std::time_t
        std::time_t time_t_value = std::mktime(&tm);

        // Step 3: Convert std::time_t to std::chrono::system_clock::time_point
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(time_t_value);

        // Step 4: Add fractional seconds (e.g., milliseconds)
        if (datetime_str.size() > 19) {
            auto milliseconds = std::stoi(datetime_str.substr(20));
            tp += std::chrono::milliseconds(milliseconds);
        }

        return tp;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return std::nullopt;
    }
}