#include "main_window.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder)
    : Gtk::Window(obj),
      m_builder(refBuilder)
{
    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("main_window", root);
    root->set_title("Eagle Eye");
}

MainWindow::~MainWindow()
{
}