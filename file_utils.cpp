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