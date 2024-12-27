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
    Gtk::DrawingArea *m_report_image_display_area;
    Gtk::DrawingArea *m_report_timeline_drawing_area;
    Gtk::ListBox *m_report_transactions_listbox;
    Gtk::Switch *m_report_masking_switch;
    Glib::RefPtr<Gdk::Pixbuf> m_image_pixbuf_detection_result;
    Glib::RefPtr<Gdk::Pixbuf> m_mask_pixbuf_detection_result;

    void on_window_shown();
    void on_report_time_range_selector_changed();
    void on_enable_masking_changed();
    void on_transaction_selected(Gtk::ListBoxRow* row);
    bool on_report_image_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_report_timeline_draw(const Cairo::RefPtr<Cairo::Context> &cr);

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    bool m_show_mask_detection_result = false;
    double m_mask_alpha = 0.5;

    void load_detection_result(std::string &detection_result_folder);
    void update_mask_color(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf);
    void update_mask_alpha(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf, gint32 alpha);
    std::chrono::system_clock::time_point parse_time(const std::string &time_str);

};

#endif