#ifndef EAGLE_EYE_MAIN_WINDOW_H
#define EAGLE_EYE_MAIN_WINDOW_H

#include <gtkmm.h>

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder);
    virtual ~MainWindow();

protected:

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
};

#endif