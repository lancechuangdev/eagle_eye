#include "app_paths.h"
#include "main_window.h"
#include "settings_service.h"
#include "retention_manager.h"

int main(int argc, char **argv)
{
    // Create a shared Logger instance
    std::shared_ptr<Logger> logger = std::make_shared<Logger>(AppPaths::Log_File_Path.string());
    logger->log("app started");

    auto app = Gtk::Application::create(argc, argv, "com.example.eagle-eye");
    auto builder = Gtk::Builder::create();

    try
    {
        auto gladeFile = FileUtils::getGladeFilePath();
        builder->add_from_file(gladeFile);
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "FileError: " << ex.what() << std::endl;
        logger->log("FileError: " + std::string(ex.what()), Logger::ERROR);
        return 1;
    }
    catch (const Glib::MarkupError &ex)
    {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
        logger->log("MarkupError: " + std::string(ex.what()), Logger::ERROR);
        return 1;
    }
    catch (const Gtk::BuilderError &ex)
    {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
        logger->log("BuilderError: " + std::string(ex.what()), Logger::ERROR);
        return 1;
    }

    // Initialize MV camera control.
    int nRet = MV_CC_Initialize();
    if (nRet != MV_OK)
    {
        logger->log("Error to initialize MV SDK", Logger::ERROR);
        std::cout << "Initialize SDK fail!" << std::endl;
    }

    // Load top level window from glade.
    MainWindow *wnd = nullptr;
    builder->get_widget_derived("main_window", wnd, logger);

    // Apply Retention Policy
    // RetentionManager retention_manager;
    // retention_manager.archive();
    // retention_manager.enforce_archive_retention();
    // retention_manager.enforce_daily_limit();

    // Shows the window and returns when it is closed.
    nRet = app->run(*wnd);

    // Ensure MV_CC_Finalize is called after the window is closed
    MV_CC_Finalize();

    return nRet;
}