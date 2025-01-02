#ifndef EAGLE_EYE_REPORT_WINDOW_H
#define EAGLE_EYE_REPORT_WINDOW_H

#include <gtkmm.h>


class ReportWindow : public Gtk::Window
{
public:
    ReportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    static ReportWindow *create(const std::string &gladeFailePath);

protected:
    Gtk::ComboBoxText *m_report_time_range_selector_cbox;
    Gtk::ProgressBar *m_report_progress_bar;
    Gtk::Label *m_report_start_time_lbl;
    Gtk::Label *m_report_end_time_lbl;
    Gtk::DrawingArea *m_report_image_display_area;
    Gtk::DrawingArea *m_report_timeline_drawing_area;
    Gtk::ListBox *m_report_transactions_listbox;
    Gtk::Switch *m_report_masking_switch;
    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_detection_result;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_detection_result;
    Gtk::Label *m_detection_result_datetime_lbl;
    Gtk::Label *m_detection_result_path_lbl;
    Gtk::Entry *m_moving_speed_entry;
    Gtk::Button *m_save_report_btn;

    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    void on_window_shown();
    void on_report_time_range_selector_changed();
    void on_enable_masking_changed();
    void on_transaction_selected(Gtk::ListBoxRow* row);
    bool on_report_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_report_display_area_scroll_event(GdkEventScroll *scroll_event);
    bool on_report_display_area_btn_press_event(GdkEventButton *button_event);
    bool on_report_display_area_btn_release_event(GdkEventButton *button_event);
    bool on_report_display_area_motion_notify_event(GdkEventMotion *motion_event);
    bool on_report_timeline_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    void on_save_report_clicked();
    
private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    bool m_show_mask_detection_result = false;
    std::vector<std::filesystem::path> m_detection_results_in_report;
    std::string m_selected_detection_result_in_report;
    bool m_ctrl_pressed = false; // Flag to check if Ctrl key is pressed
    double m_mask_alpha = 0.5;
    double m_zoom_factor = 1.0; // Zoom factor (1.0 = no zoom)
    bool m_is_dragging = false; // Track whether the user is dragging on report display area
    double m_drag_start_x = 0.0; // Mouse drag start X on report display area
    double m_drag_start_y = 0.0; // Mouse drag start Y on report display area
    double m_offset_x = 0.0; // Horizontal pan offset on report display area
    double m_offset_y = 0.0; // Vertical pan offset on report display area

    void load_detection_result(std::string &detection_result_folder);
    void update_mask_color(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf);
    void update_mask_alpha(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf, gint32 alpha);
    std::chrono::system_clock::time_point parse_time(const std::string &time_str);

};

#endif