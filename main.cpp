#include "main_window.h"

int main(int argc, char **argv)
{
    auto app = Gtk::Application::create(argc, argv, "com.example.eagle-eye");
    auto builder = Gtk::Builder::create();

    try
    {
        // auto gladeFile = FileUtils::getGladeFilePath();
        // builder->add_from_file(gladeFile);
        builder->add_from_file("../ui.glade");
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "FileError: " << ex.what() << std::endl;
        return 1;
    }
    catch (const Glib::MarkupError &ex)
    {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
        return 1;
    }
    catch (const Gtk::BuilderError &ex)
    {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
        return 1;
    }

    // Initialize MV camera control.
    int nRet = MV_CC_Initialize();
    if (nRet != MV_OK)
    {
        // logger->log("Error to initialize MV SDK", Logger::ERROR);
        std::cout << "Initialize SDK fail!" << std::endl;
    }

    // Load top level window from glade.
    MainWindow *wnd = nullptr;
    builder->get_widget_derived("main_window", wnd);

    // Shows the window and returns when it is closed.
    nRet = app->run(*wnd);

    // Ensure MV_CC_Finalize is called after the window is closed
    MV_CC_Finalize();

    return nRet;
}