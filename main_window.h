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
#include <queue>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include "MvCameraControl.h"
#include "frame_queue.h"
#include "pixbuf_queue.h"
#include "logger.h"
#include "file_utils.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger);
    virtual ~MainWindow();

protected:
    Gtk::Button *m_start_signal_test_btn;
    Gtk::ComboBoxText *m_runtime_mode_cbox;
    Gtk::RadioButton *m_startup_btn;
    Gtk::RadioButton *m_runtime_btn;
    Gtk::RadioButton *m_explore_btn;
    Gtk::RadioButton *m_toolkit_btn;
    Gtk::RadioButton *m_settings_btn;
    Gtk::RadioButton *m_runtime_control_panel_rbtn;
    Gtk::RadioButton *m_runtime_monitoring_rbtn;
    Gtk::RadioButton *m_runtime_report_rbtn;
    Gtk::Stack *m_runtime_stack;
    Gtk::Box *m_runtime_start_section_box;
    Gtk::Box *m_runtime_recent_section_box;
    Gtk::Box *m_runtime_close_section_box;
    Gtk::Button *m_new_project_btn;
    Gtk::Button *m_open_project_btn;
    Gtk::Button *m_close_project_btn;
    Gtk::Button *m_quick_start_btn; // start without project
    Gtk::ListBox *m_recent_projects_listbox;
    Gtk::Label *m_runtime_no_project_lbl;
    Gtk::ButtonBox *m_runtime_nav_button_box;
    Gtk::DrawingArea *m_rt_monitoring_drawing_area;
    Gtk::Label *m_rt_monitoring_detection_start_time_lbl;
    Gtk::Label *m_rt_monitoring_num_anomalies_lbl;
    Gtk::Label *m_detection_camera_lbl;
    Gtk::Label *m_detection_rate_lbl;
    Gtk::Label *m_detection_digital_input_lbl;
    Gtk::Label *m_detection_digital_input_line_number_lbl;
    Gtk::Label *m_detection_digital_output_lbl;
    Gtk::Label *m_detection_digital_output_line_number_lbl;
    Gtk::Button *m_start_btn;
    Gtk::Button *m_stop_btn;
    Gtk::TextView *m_runtime_event_viewer;
    Gtk::Button *m_snap_btn;
    Gtk::ComboBoxText *m_snap_source_cbox;
    Gtk::FileChooserButton *m_toolkit_image_picker_fcb;
    Gtk::Button *m_toolkit_detection_test_btn;
    Gtk::DrawingArea *m_toolkit_display_area;
    Gtk::Stack *m_content_stack;
    Gtk::Grid *m_cam_grid;
    Gtk::DrawingArea *m_settings_display_area;
    Gtk::Grid *m_cam_settings_grid;
    Gtk::Label *m_sn_lbl;
    Gtk::Entry *m_exposure_time_entry;
    Gtk::SpinButton *m_width_sb;
    Gtk::SpinButton *m_height_sb;
    Gtk::SpinButton *m_offset_x_sb;
    Gtk::SpinButton *m_offset_y_sb;
    Gtk::ComboBoxText *m_digital_io_type_cbox;
    Gtk::Grid *m_digital_input_grid;
    Gtk::Grid *m_digital_output_grid;
    Gtk::ComboBoxText *m_settings_digital_input_line_number_cbox;
    Gtk::SpinButton *m_settings_digital_input_debouncer_time_sb;
    Gtk::ComboBoxText *m_settings_digital_input_event_trigger_cbox;
    Gtk::ComboBoxText *m_settings_digital_input_notification_status_cbox;
    Gtk::ComboBoxText *m_settings_digital_output_line_number_cbox;
    Gtk::ComboBoxText *m_settings_digital_output_line_mode_cbox;
    Gtk::ComboBoxText *m_settings_digital_output_line_source_cbox;
    Gtk::Switch *m_settings_strobe_enable_switch;
    Gtk::SpinButton *m_settings_strobe_duration_sb;
    Glib::RefPtr<Gtk::Adjustment> m_width_adj;
    Glib::RefPtr<Gtk::Adjustment> m_height_adj;
    Glib::RefPtr<Gtk::Adjustment> m_offset_x_adj;
    Glib::RefPtr<Gtk::Adjustment> m_offset_y_adj;
    Glib::RefPtr<Gtk::Adjustment> m_debounce_time_adj;
    Gtk::Button *m_save_camera_settings_btn;
    Gtk::Stack *m_toolkit_stack;
    Gtk::RadioButton *m_toolkit_anomaly_detection_rbtn;
    Gtk::RadioButton *m_toolkit_digital_io_rbtn;
    Gtk::ComboBoxText *m_toolkit_digital_output_source_cbox;
    Gtk::ComboBoxText *m_toolkit_digital_output_line_number_cbox;
    Gtk::Switch *m_toolkit_strobe_enable_switch;
    Glib::RefPtr<Gtk::Adjustment> m_strobe_duration_adj;
    Gtk::SpinButton *m_toolkit_strobe_duration_sb;
    Gtk::Button *m_toolkit_test_digital_out_btn;
    Gtk::ComboBoxText *m_toolkit_digital_input_sink_cbox;
    Gtk::ComboBoxText *m_toolkit_digital_input_line_number_cbox;
    Gtk::SpinButton *m_toolkit_digital_input_debounce_time_sb;
    Gtk::ComboBoxText *m_toolkit_digital_input_event_trigger_cbox;
    Gtk::ComboBoxText *m_toolkit_digital_input_notification_status_cbox;
    Gtk::Button *m_start_digital_input_event_listening_btn;
    Gtk::Button *m_stop_digital_input_event_listening_btn;
    Gtk::Label *m_digital_input_event_status_lbl;
    Gtk::TextView *m_digital_input_event_tv;
    Gtk::Stack *m_settings_stack;
    Gtk::RadioButton *m_anomaly_detection_settings_rbtn;
    Gtk::RadioButton *m_camera_settings_rbtn;
    Gtk::ComboBoxText *m_select_detection_camera_cbox;
    Gtk::ComboBoxText *m_select_detection_digital_input_cbox;
    Gtk::Label *m_settings_detection_digital_input_line_number_lbl;
    Gtk::ComboBoxText *m_select_detection_digital_output_cbox;
    Gtk::Label *m_settings_detection_digital_output_line_number_lbl;
    Gtk::Scale *m_detection_sensitivity_scale;
    Gtk::Button *m_cancel_detection_settings_btn;
    Gtk::Button *m_save_detection_settings_btn;
    Gtk::Spinner *m_load_detection_results_spinner;
    Gtk::ListBox *m_detection_results_listbox;
    Gtk::Box *m_detection_patches_box;
    Gtk::Label *m_current_selected_patch_in_explorer_lbl = nullptr;
    Gtk::DrawingArea *m_detection_results_display_area;
    Gtk::ComboBoxText *m_recent_detection_results_selector_cbox;
    Gtk::Button *m_detection_results_refresh_btn;
    Gtk::Switch *m_detection_results_masking_switch;
    Gtk::Entry *m_moving_speed_entry;
    Gtk::Button *m_save_report_btn;
    Gtk::Button *m_view_report_btn;
    Gtk::Button *m_report_refresh_btn;
    Gtk::Spinner *m_load_report_spinner;
    Gtk::ListBox *m_report_transactions_listbox;
    Gtk::Switch *m_report_masking_switch;
    Gtk::DrawingArea *m_report_image_display_area;
    Gtk::Box *m_report_patches_box;
    Gtk::Label* m_current_selected_patch_in_report_lbl;
    Gtk::DrawingArea *m_report_position_display_area;

    void on_start_signal_test_clicked();
    void on_window_shown();
    bool on_window_delete(GdkEventAny* event);
    void on_menu_toggled();
    void on_runtime_mode_changed();
    void on_runtime_tab_clicked();
    void on_new_project_clicked();
    void on_open_project_clicked();
    void on_close_project_clicked();
    void on_quick_start_clicked();
    void on_recent_project_selected(Gtk::ListBoxRow* row);
    void on_start_clicked();
    void on_stop_clicked();
    void on_snap_clicked();
    void on_toolkit_test_clicked();
    void on_connect_clicked(const std::string& sn);
    void on_disconnect_clicked(const std::string& sn);
    void on_view_clicked(const std::string& sn);
    void on_report_refresh_clicked();
    void on_transaction_selected(Gtk::ListBoxRow* row);
    bool on_report_position_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_report_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_report_display_area_scroll_event(GdkEventScroll *scroll_event);
    bool on_report_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_report_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_report_display_area_motion_notify_event(GdkEventMotion *motion_event);
    void on_report_enable_masking_changed();
    void on_save_report_clicked();
    void on_view_report_clicked();
    bool on_rt_monitoring_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_toolkit_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_settings_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_exposure_time_entry_focus_out(GdkEventFocus* event);
    void on_width_value_changed();
    void on_height_value_changed();
    void on_offset_x_value_changed();
    void on_offset_y_value_changed();
    void on_digital_io_type_changed();
    void on_digital_input_line_number_changed();
    void on_digital_output_line_number_changed();
    void on_digital_output_line_mode_changed();
    void on_digital_output_line_source_changed();
    void on_strobe_enable_state_set();
    void on_strobe_duration_value_changed();
    void on_test_digital_out_clicked();
    void on_start_listening_for_start_signal_clicked();
    void on_stop_listening_for_start_signal_clicked();
    void on_save_camera_settings_clicked();
    void on_toolkit_toggled();
    void on_settings_toggled();
    void on_cancel_detection_settings_clicked();
    void on_save_detection_settings_clicked();
    void on_detection_result_selected(Gtk::ListBoxRow* row);
    bool on_detection_results_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    void on_detection_results_refresh_clicked();
    void on_recent_detection_results_selector_changed();
    void on_enable_masking_changed();
    void on_detection_digital_input_selection_changed();
    void on_detection_digital_output_selection_changed();

    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    // Mouse events
    bool on_rt_monitoring_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_rt_monitoring_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_rt_monitoring_display_area_motion_notify_event(GdkEventMotion *motion_event);
    bool on_rt_monitoring_display_area_scroll_event(GdkEventScroll *scroll_event);
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
    struct CaptureCallbackData
    {
        MainWindow *main_window_ptr;
        std::string serial_number;

        CaptureCallbackData(MainWindow *ptr, const std::string sn)
            : main_window_ptr(ptr), serial_number(sn) {}
    };

    struct RuntimeEvent
    {
        std::string message;
        std::string datetime;
        std::string color;
    };

    struct CameraEvent
    {
        std::string event_name;
        int event_id;
    };
    

    Glib::RefPtr<Gtk::Builder> m_builder;
    std::string m_curr_project_name;
    std::string m_runtime_mode;
    MV_CC_DEVICE_INFO_LIST m_cam_list;
    std::unordered_map<std::string, void*> m_connected_device_handles;
    std::atomic<bool> m_is_running;    
    std::atomic<bool> m_is_listening_toolkit_digital_input_event;
    
    std::atomic<bool> m_is_monitoring_camera_event;
    std::atomic<bool> m_stop_processing_camera_event;
    std::queue<CameraEvent> m_cam_event_queue;
    std::condition_variable m_cam_event_queue_cv;
    std::mutex m_cam_event_queue_mutex;

    FrameQueue m_frame_queue;
    PixbufQueue m_pixbuf_queue;
    std::unordered_map<std::string, std::thread> m_capturing_threads;
    std::thread m_processing_thread;
    std::thread m_warmup_thread;
    size_t m_session_anomaly_count;
    std::vector<std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point>> m_session_times;

    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_rt_monitoring;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_rt_monitoring;
    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_report;
    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_report_original;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_report;
    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_toolkit;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_toolkit;
    double m_mask_alpha = 0.5;
    bool m_ctrl_pressed = false; // Flag to check if Ctrl key is pressed

    bool m_is_dragging_rt_monitoring = false; // Track whether the user is dragging on rt monitoring page
    double m_drag_start_x_rt_monitoring = 0.0; // Mouse drag start X on rt monitoring page
    double m_drag_start_y_rt_monitoring = 0.0; // Mouse drag start Y on rt monitoring page
    double m_offset_x_rt_monitoring = 0.0;    // Horizontal pan offset on rt monitoring page
    double m_offset_y_rt_monitoring = 0.0;    // Vertical pan offset on rt monitoring page
    double m_zoom_factor_rt_monitoring = 1.0; // Zoom factor (1.0 = no zoom) on rt monitoring page

    bool m_is_dragging_report = false; // Track whether the user is dragging on report page
    double m_drag_start_x_report = 0.0; // Mouse drag start X on report page
    double m_drag_start_y_report = 0.0; // Mouse drag start Y on report page
    double m_offset_x_report = 0.0;    // Horizontal pan offset on report page
    double m_offset_y_report = 0.0;    // Vertical pan offset on report page
    double m_zoom_factor_report = 1.0; // Zoom factor (1.0 = no zoom) on report page

    bool m_is_dragging_toolkit = false; // Track whether the user is dragging on toolkit page
    double m_drag_start_x_toolkit = 0.0; // Mouse drag start X on toolkit page
    double m_drag_start_y_toolkit = 0.0; // Mouse drag start Y on toolkit page
    double m_offset_x_toolkit = 0.0;    // Horizontal pan offset on toolkit page
    double m_offset_y_toolkit = 0.0;    // Vertical pan offset on toolkit page
    double m_zoom_factor_toolkit = 1.0; // Zoom factor (1.0 = no zoom) on toolkit page

    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_settings;
    bool m_is_dragging_settings = false; // Track whether the user is dragging on settings page
    double m_drag_start_x_settings = 0.0; // Mouse drag start X on settings page
    double m_drag_start_y_settings = 0.0; // Mouse drag start Y on settings page
    double m_offset_x_settings = 0.0;    // Horizontal pan offset on settings page
    double m_offset_y_settings = 0.0;    // Vertical pan offset on settings page
    double m_zoom_factor_settings = 1.0; // Zoom factor (1.0 = no zoom) on settings page

    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_explorer;
    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_explorer_original;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_explorer;
    bool m_show_mask_detection_result;
    bool m_is_dragging_detection = false; // Track whether the user is dragging on detection results page
    double m_drag_start_x_detection = 0.0; // Mouse drag start X on detection results page
    double m_drag_start_y_detection = 0.0; // Mouse drag start Y on detection results page
    double m_offset_x_detection = 0.0;    // Horizontal pan offset on detection results page
    double m_offset_y_detection = 0.0;    // Vertical pan offset on detection results page
    double m_zoom_factor_detection = 1.0; // Zoom factor (1.0 = no zoom) on detection results page

    Glib::RefPtr<Gio::FileMonitor> m_detection_results_monitor;
    std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, double>> m_transactions_with_positions;
    std::string m_selected_transaction_id;
    bool m_show_mask_in_report;
    std::shared_ptr<Logger> m_logger;
    Glib::Dispatcher m_main_images_dispatcher;
    Glib::Dispatcher m_main_masks_dispatcher;
    sigc::connection m_images_dispatcher_connection;
    sigc::connection m_masks_dispatcher_connection;
    std::atomic<bool> m_images_dispatcher_running = false;
    std::atomic<bool> m_masks_dispatcher_running = false;

    std::vector<uint8_t> m_frame_rgb_data_buffer;
    std::vector<uint8_t> m_patch_rgba_data_buffer;

    Glib::Dispatcher m_event_dispatcher; // Dispatcher to signal the main thread
    std::vector<RuntimeEvent> m_pending_events; // Thread-safe event storage
    std::mutex m_event_mutex; // Mutex to protect the event list

    std::vector<double> m_detection_rate_records;
    std::mutex m_detection_rate_mutex;

    std::unique_ptr<Ort::Session> m_onnx_detection_session;
    std::unique_ptr<Ort::Session> m_onnx_toolkit_session;

    void set_window_title(const std::string &title);
    bool create_project(const std::string &project_name);
    void open_project(const std::string &project_file_path);
    void update_runtime_page(const std::string &option);
    void update_startup_page(bool show_start = true);
    void discover_cameras();
    bool connect_camera(const std::string& sn);
    bool disconnect_camera(const std::string& sn);
    bool configure_camera(const std::string sn);
    void update_cam_grid();
    void show_dialog(Gtk::Window& parent, std::string message, std::string secondary_message="", Gtk::MessageType message_type = Gtk::MESSAGE_ERROR);
    void *create_or_get_device_handle_by_serial_number(std::string sn);
    void start_capture(void *device_handle);
    void start_warmup(int frame_width, int frame_height);
    void start_detection(int frame_width, int frame_height);
    void start_detection_with_warmup(int frame_width, int frame_height);
    void stop_capture(void *device_handle);
    void stop_detection();
    void start_detection_rate_timer();
    void on_event_dispatch(); // Called when dispatcher emits a signal
    void add_runtime_event(const std::string &message, const std::string &color="");
    void clear_runtime_events();
    std::string generate_transaction_id();
    // void update_mask_color(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf);
    void update_mask_alpha(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf, gint32 alpha);
    std::string convert_to_ip_address_str(uint32_t ip);
    void set_button_icon(Gtk::Button* button, const Glib::ustring& resource_path);
    void populate_camera_settings(void *device_handle);
    void clear_camera_settings();
    void snap_and_display(void *device_handle);
    std::string run_command(const std::string& command);
    void load_recent_projects();
    void load_detection_settings();
    std::vector<std::filesystem::path> scan_filesystem_for_detection_results(
        const std::chrono::system_clock::time_point& start_tp,
        const std::chrono::system_clock::time_point& end_tp);
    void load_detection_results_async();
    void load_detection_result_in_explorer(std::string &detection_result_folder);
    void load_detection_result_in_report(std::string &detection_result_folder);
    std::string get_patch_remark(const std::string &transaction_json_path, int prediction_id);
    void update_patch_thumbnail_alpha(Glib::RefPtr<Gdk::Pixbuf> thumbnail_pixbuf, Gtk::Image *thumbnail, int alpha_value);
    void update_patch_remark(const std::string &transaction_json_path, int prediction_id, const std::string &remark);
    void update_track_positions(double speed);
    std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, double>> track_position(double speed);
    bool is_transaction_valid(const std::string &transaction_path);
    void create_csv_file(const std::string &file_name);
    void start_listening_for_start_signal();
    void stop_listening_for_start_signal();
    void process_camera_event();
    void setup_onnx_detection_session(bool enable_cache);
    void setup_onnx_toolkit_session();
    std::vector<Ort::Value> run_inference(const std::unique_ptr<Ort::Session>& session, std::vector<Ort::Value>& input_tensors);
    void print_tensor_shape(const Ort::Value& tensor, const std::string& tensor_name);
    void print_tensor_values(const Ort::Value& tensor, const std::string& name);
    void save_pixbuf(const cv::Mat& pixbuf, const std::string& file_path);
};

#endif