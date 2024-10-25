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
    Gtk::SpinButton *m_captureRateSb;
    Gtk::FileChooserButton *m_load_model_fcb;
    Gtk::FileChooserButton *m_prediction_result_fcb;
    Gtk::Button *m_preflight_btn;
    Gtk::Button *m_start_btn;

    void on_window_shown();
    void on_preflight_clicked();
    void on_start_clicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    MV_CC_DEVICE_INFO_LIST m_camList;
    void discover_cameras();
};

#endif