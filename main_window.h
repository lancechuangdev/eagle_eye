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
#include <nlohmann/json.hpp>
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
    Gtk::Button *m_start_btn;
    Gtk::Button *m_stop_btn;

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
    struct FrameOffsetInfo
    {
        size_t offset;
        size_t frame_size;
        size_t frame_width;
        size_t frame_height;
    };
    // FILE *m_pipe;

    void discover_cameras();
    void *get_device_handle_by_serial_number(std::string sn);
    std::vector<void*> get_all_device_handles();
    void preflight(void *device_handle);
    void start_capture(void *device_handle, double capture_interval_ms);
    void start_detection();
    void predict(std::string command);
    void stop_capture(void *device_handle);
    void send_ws_message(std::string message);

    WebSocketClient m_ws_client;
    bool m_is_ws_connected;
    std::string m_ws_response;
    std::mutex m_ws_response_mutex;
    std::condition_variable m_ws_response_cv;
    bool m_ws_response_ready = false; // Condition to wait on

    std::shared_ptr<Logger> m_logger;
};

#endif