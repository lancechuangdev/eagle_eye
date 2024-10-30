#ifndef EAGLE_EYE_MAIN_WINDOW_H
#define EAGLE_EYE_MAIN_WINDOW_H

#include <gtkmm.h>
#include <iostream>
#include <sys/mman.h> // For shm_open, mmap, etc.
#include <fcntl.h>    // For O_* constants
#include <unistd.h>   // For ftruncate, close.
#include <cstring>    // For memcpy
#include <atomic>
#include <chrono>
#include <thread>
#include <future>
#include <unordered_map>
#include "MvCameraControl.h"
#include "frame_queue.h"
#include "pyscript.h"
#include "logger.h"
#include "file_utils.h"
#include "web_socket_client.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger);
    virtual ~MainWindow();

protected:
    Gtk::ComboBoxText *m_camera_combo_box;
    Gtk::SpinButton *m_capture_rate_sb;
    Gtk::FileChooserButton *m_load_model_fcb;
    Gtk::FileChooserButton *m_prediction_result_fcb;
    Gtk::SpinButton *m_confidence_threshold_sb;
    Gtk::SpinButton *m_pixel_threshold_sb;
    Gtk::Button *m_start_btn;
    Gtk::Button *m_stop_btn;
    Gtk::Entry *m_py_env_entry;
    Gtk::SpinButton *m_patch_size_sb;

    void on_window_shown();
    bool on_window_delete(GdkEventAny* event);
    void on_start_clicked();
    void on_stop_clicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    MV_CC_DEVICE_INFO_LIST m_camList;
    std::vector<void*> m_device_handles;
    std::atomic<bool> m_is_capturing;
    FrameQueue m_frame_queue;
    std::string m_py_script;
    std::unordered_map<std::string, std::thread> m_capturing_threads;
    std::thread m_processing_thread;

    void discover_cameras();
    void *get_device_handle_by_serial_number(std::string sn);
    std::vector<void*> get_all_device_handles();
    void preflight(void *device_handle);
    void start_capture(void *device_handle, double capture_interval_ms);
    void start_detection(std::string py_script_path);
    void predict(std::string command);
    void stop_capture(void *device_handle);

    WebSocketClient m_ws_client;
    bool m_is_ws_connected;
    std::shared_ptr<Logger> m_logger;
};

#endif