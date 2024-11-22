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
    Gtk::RadioButton *m_toolkit_btn;
    Gtk::RadioButton *m_run_btn;
    Gtk::RadioButton *m_explore_btn;
    Gtk::RadioButton *m_settings_btn;
    Gtk::ComboBoxText *m_detection_source_cbox;
    Gtk::SpinButton *m_capture_rate_sb;
    Gtk::Button *m_start_btn;
    Gtk::Button *m_stop_btn;
    Gtk::Button *m_snap_btn;
    Gtk::ComboBoxText *m_snap_source_cbox;
    Gtk::DrawingArea *m_toolkit_display_area;
    Gtk::Stack *m_content_stack;
    Gtk::Grid *m_cam_grid;
    Gtk::DrawingArea *m_settings_display_area;
    Gtk::Label *m_sn_lbl;
    Gtk::Entry *m_exposure_time_entry;
    Gtk::SpinButton *m_width_sb;
    Gtk::SpinButton *m_height_sb;
    Gtk::SpinButton *m_offset_x_sb;
    Gtk::SpinButton *m_offset_y_sb;
    Gtk::ComboBoxText *m_settings_digital_io_line_number_cbox;
    Gtk::ComboBoxText *m_settings_digital_io_line_mode_cbox;
    Gtk::ComboBoxText *m_settings_digital_io_line_source_cbox;
    Gtk::Switch *m_settings_strobe_enable_switch;
    Gtk::SpinButton *m_settings_strobe_duration_sb;
    Glib::RefPtr<Gtk::Adjustment> m_width_adj;
    Glib::RefPtr<Gtk::Adjustment> m_height_adj;
    Glib::RefPtr<Gtk::Adjustment> m_offset_x_adj;
    Glib::RefPtr<Gtk::Adjustment> m_offset_y_adj;
    Gtk::Button *m_save_camera_settings_btn;
    Gtk::Stack *m_toolkit_stack;
    Gtk::RadioButton *m_toolkit_anomaly_detection_rbtn;
    Gtk::RadioButton *m_toolkit_digital_io_rbtn;
    Gtk::Button *m_check_service_status_btn;
    Gtk::Label *m_service_status_lbl;
    Gtk::ComboBoxText *m_toolkit_capture_source_cbox;
    Gtk::ComboBoxText *m_toolkit_digital_io_line_number_cbox;
    Gtk::Switch *m_toolkit_strobe_enable_switch;
    Glib::RefPtr<Gtk::Adjustment> m_strobe_duration_adj;
    Gtk::SpinButton *m_toolkit_strobe_duration_sb;
    Gtk::Button *m_toolkit_test_digital_out_btn;
    Gtk::Stack *m_settings_stack;
    Gtk::RadioButton *m_anomaly_detection_settings_rbtn;
    Gtk::RadioButton *m_camera_settings_rbtn;
    Gtk::Scale *m_detection_sensitivity_scale;
    Gtk::Scale *m_anomaly_size_threshold_scale;
    Gtk::Button *m_cancel_detection_settings_btn;
    Gtk::Button *m_save_detection_settings_btn;
    Gtk::ListBox *m_detection_results_listbox;
    Gtk::DrawingArea *m_detection_results_display_area;
    Gtk::ComboBoxText *m_recent_detection_results_selector_cbox;
    Gtk::Button *m_detection_results_refresh_btn;
    Gtk::Label *m_last_detection_results_refresh_time_lbl;
    Gtk::Label *m_detection_results_path_lbl;
    Gtk::SpinButton *m_max_per_day_sb;
    Gtk::SpinButton *m_days_to_retain_sb;
    Gtk::Label *m_detection_results_memory_usage_lbl;
    Gtk::Button *m_delete_detection_results_btn;

    void on_window_shown();
    bool on_window_delete(GdkEventAny* event);
    void on_menu_toggled();
    void on_start_clicked();
    void on_stop_clicked();
    void on_snap_clicked();
    void on_connect_clicked(const std::string& sn);
    void on_disconnect_clicked(const std::string& sn);
    void on_view_clicked(const std::string& sn);
    bool on_toolkit_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_settings_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_exposure_time_entry_focus_out(GdkEventFocus* event);
    void on_width_value_changed();
    void on_height_value_changed();
    void on_offset_x_value_changed();
    void on_offset_y_value_changed();
    void on_digital_io_line_number_changed();
    void on_digital_io_line_mode_changed();
    void on_digital_io_line_source_changed();
    bool on_strobe_enable_state_set(bool state);
    void on_strobe_duration_value_changed();
    void on_save_camera_settings_clicked();
    void on_toolkit_toggled();
    void on_check_service_status_clicked();
    void on_test_digital_out_clicked();
    void on_settings_toggled();
    void on_cancel_detection_settings_clicked();
    void on_save_detection_settings_clicked();
    void on_detection_result_selected(Gtk::ListBoxRow* row);
    bool on_detection_results_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    void on_detection_results_refresh_clicked();
    void on_recent_detection_results_selector_changed();
    void on_delete_detection_results_clicked();

    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    // Mouse events
    bool on_detection_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_detection_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_detection_display_area_motion_notify_event(GdkEventMotion *motion_event);
    bool on_detection_display_area_scroll_event(GdkEventScroll *scroll_event);
    bool on_toolkit_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_toolkit_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_toolkit_display_area_motion_notify_event(GdkEventMotion *motion_event);
    bool on_toolkit_display_area_scroll_event(GdkEventScroll *scroll_event);
    bool on_settings_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_settings_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_settings_display_area_motion_notify_event(GdkEventMotion *motion_event);
    bool on_settings_display_area_scroll_event(GdkEventScroll *scroll_event);

private:
    struct FrameOffsetInfo
    {
        size_t offset;
        size_t frame_size;
        std::string serial_number;
    };

    struct CaptureCallbackData
    {
        MainWindow *main_window_ptr;
        std::string serial_number;

        CaptureCallbackData(MainWindow *ptr, const std::string sn)
            : main_window_ptr(ptr), serial_number(sn) {}
    };

    Glib::RefPtr<Gtk::Builder> m_builder;
    MV_CC_DEVICE_INFO_LIST m_cam_list;
    std::unordered_map<std::string, void*> m_connected_device_handles;
    std::atomic<bool> m_is_running;
    FrameQueue m_frame_queue;
    std::unordered_map<std::string, std::thread> m_capturing_threads;
    std::thread m_processing_thread;

    WebSocketClient m_ws_client;
    bool m_is_ws_connected;
    std::string m_ws_response;
    std::mutex m_ws_response_mutex;
    std::condition_variable m_ws_response_cv;
    bool m_ws_response_ready = false; // Condition to wait on

    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_toolkit;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_toolkit;
    double m_mask_alpha = 0.5;
    bool m_ctrl_pressed = false; // Flag to check if Ctrl key is pressed
    bool m_is_dragging_toolkit = false; // Track whether the user is dragging on test page
    double m_drag_start_x_toolkit = 0.0; // Mouse drag start X on test page
    double m_drag_start_y_toolkit = 0.0; // Mouse drag start Y on test page
    double m_offset_x_toolkit = 0.0;    // Horizontal pan offset on test page
    double m_offset_y_toolkit = 0.0;    // Vertical pan offset on test page
    double m_zoom_factor_toolkit = 1.0; // Zoom factor (1.0 = no zoom) on test page

    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_settings;
    bool m_is_dragging_settings = false; // Track whether the user is dragging on settings page
    double m_drag_start_x_settings = 0.0; // Mouse drag start X on settings page
    double m_drag_start_y_settings = 0.0; // Mouse drag start Y on settings page
    double m_offset_x_settings = 0.0;    // Horizontal pan offset on settings page
    double m_offset_y_settings = 0.0;    // Vertical pan offset on settings page
    double m_zoom_factor_settings = 1.0; // Zoom factor (1.0 = no zoom) on settings page

    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_detection_result;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_detection_result;
    bool m_is_dragging_detection = false; // Track whether the user is dragging on detection results page
    double m_drag_start_x_detection = 0.0; // Mouse drag start X on detection results page
    double m_drag_start_y_detection = 0.0; // Mouse drag start Y on detection results page
    double m_offset_x_detection = 0.0;    // Horizontal pan offset on detection results page
    double m_offset_y_detection = 0.0;    // Vertical pan offset on detection results page
    double m_zoom_factor_detection = 1.0; // Zoom factor (1.0 = no zoom) on detection results page

    Glib::RefPtr<Gio::FileMonitor> m_detection_results_monitor;
    std::chrono::steady_clock::time_point m_last_load_time;
    std::shared_ptr<Logger> m_logger;

    void discover_cameras();
    bool connect_camera(const std::string& sn);
    bool disconnect_camera(const std::string& sn);
    bool configure_camera(const std::string sn);
    void update_cam_grid();
    void show_camera_connect_warning(Gtk::Window& parent, std::string message);
    void *create_or_get_device_handle_by_serial_number(std::string sn);
    void start_capture(void *device_handle, double capture_interval_ms);
    void start_detection();
    void stop_capture(void *device_handle);
    void stop_detection();
    void send_ws_message(std::string message);
    void save_tmp_image(unsigned char *pData, MV_FRAME_OUT_INFO_EX FrameInfo, void *deviceHandle);
    std::string generate_transaction_id();
    void update_mask_color(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf);
    void update_mask_alpha(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf, gint32 alpha);
    void update_snap_masks(std::string trans_id);
    std::string convert_to_ip_address_str(uint32_t ip);
    void set_button_icon(Gtk::Button* button, const Glib::ustring& resource_path);
    void populate_camera_settings(void *device_handle);
    void clear_camera_settings();
    void snap_and_display(void *device_handle);
    std::string run_command(const std::string& command);
    void load_detection_settings();
    void load_detection_results();
    void load_detection_result(std::string &detection_result_folder);
    void setup_directory_monitor(const std::string &directory_path);
    void on_directory_changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &other_file, Gio::FileMonitorEvent event_type);
    double calc_detection_results_memory_usage_in_gb(size_t max_per_day, size_t days_to_retain);
    void update_detection_results_memory_usage_label(size_t max_per_day, size_t days_to_retain);
};

#endif