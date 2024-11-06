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
#include "logger.h"
#include "file_utils.h"
#include "web_socket_client.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger);
    virtual ~MainWindow();

protected:
    Gtk::RadioButton *m_verify_btn;
    Gtk::RadioButton *m_run_btn;
    Gtk::RadioButton *m_explore_btn;
    Gtk::RadioButton *m_settings_btn;
    Gtk::ComboBoxText *m_camera_combo_box;
    Gtk::SpinButton *m_capture_rate_sb;
    Gtk::Button *m_start_btn;
    Gtk::Button *m_stop_btn;
    Gtk::Button *m_snap_btn;
    Gtk::ComboBoxText *m_camera_test_combo_box;
    Gtk::DrawingArea *m_test_display_area;
    Gtk::Stack *m_content_stack;
    Gtk::Button *m_discoverBtn;
    Gtk::Grid *m_cam_grid;

    void on_window_shown();
    bool on_window_delete(GdkEventAny* event);
    void on_menu_toggled();
    void on_start_clicked();
    void on_stop_clicked();
    void on_snap_clicked();
    bool on_test_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_test_display_area_scroll_event(GdkEventScroll *scroll_event);
    void on_connect_clicked(const std::string& model);
    void on_discover_clicked();
    
    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    // Mouse events
    bool on_test_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_test_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_test_display_area_motion_notify_event(GdkEventMotion *motion_event);

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    MV_CC_DEVICE_INFO_LIST m_camList;
    std::vector<void*> m_device_handles;
    std::atomic<bool> m_is_capturing;
    FrameQueue m_frame_queue;
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
    void stop_capture(void *device_handle);
    void send_ws_message(std::string message);
    void save_image(unsigned char *pData, MV_FRAME_OUT_INFO_EX FrameInfo, void *deviceHandle);
    std::string generate_transaction_id();
    void update_mask_color();
    void update_mask_alpha(gint32 alpha);
    void display_test_masks(std::string trans_id);
    std::string convert_to_ip_address_str(uint32_t ip);
    void set_button_icon(Gtk::Button* button, const Glib::ustring& resource_path);
    void clear_grid_except_header(Gtk::Grid* grid);

    WebSocketClient m_ws_client;
    bool m_is_ws_connected;
    std::string m_ws_response;
    std::mutex m_ws_response_mutex;
    std::condition_variable m_ws_response_cv;
    bool m_ws_response_ready = false; // Condition to wait on

    Glib::RefPtr<Gdk::Pixbuf> m_ImagePixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_maskPixbuf;
    double m_mask_alpha = 0.5;
    bool m_ctrl_pressed = false; // Flag to check if Ctrl key is pressed
    bool m_is_dragging = false; // Track whether the user is dragging
    double m_drag_start_x = 0.0; // Mouse drag start X
    double m_drag_start_y = 0.0; // Mouse drag start Y
    double m_offset_x = 0.0;    // Horizontal pan offset
    double m_offset_y = 0.0;    // Vertical pan offset
    double m_zoom_factor = 1.0; // Zoom factor (1.0 = no zoom)


    std::shared_ptr<Logger> m_logger;
};

#endif