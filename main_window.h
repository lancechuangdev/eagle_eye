#ifndef EAGLE_EYE_MAIN_WINDOW_H
#define EAGLE_EYE_MAIN_WINDOW_H

#include <gtkmm.h>
#include <iostream>
#include "MvCameraControl.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder);
    virtual ~MainWindow();

protected:
    Gtk::ComboBoxText *m_cameraComboBox;
    void on_window_shown();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    MV_CC_DEVICE_INFO_LIST m_camList;
    void discover_cameras();
};

#endif