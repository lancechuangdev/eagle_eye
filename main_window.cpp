#include "main_window.h"
#include "app_paths.h"
#include "settings_service.h"
#include "retention_manager.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_frame_queue(2),
      m_logger(logger)
{
    // Create a CssProvider
    auto css_file = FileUtils::getCssFilePath();
    auto provider = Gtk::CssProvider::create();
    provider->load_from_path(css_file);

    // Apply the CSS provider to the default screen
    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(),
        provider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    signal_show().connect(sigc::mem_fun(*this, &MainWindow::on_window_shown));
    signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_window_delete));

    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("main_window", root);
    root->set_title("Eagle Eye");

    m_builder->get_widget("run_rbtn", m_run_btn);
    if (m_run_btn)
    {
        m_run_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("explore_rbtn", m_explore_btn);
    if (m_explore_btn)
    {
        m_explore_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("toolkit_rbtn", m_toolkit_btn);
    if (m_toolkit_btn)
    {
        m_toolkit_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("settings_rbtn", m_settings_btn);
    if (m_settings_btn)
    {
        m_settings_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("content_stack", m_content_stack);

    m_builder->get_widget("detection_camera_lbl", m_detection_camera_lbl);

    m_builder->get_widget("detection_rate_lbl", m_detection_rate_lbl);

    m_builder->get_widget("start_btn", m_start_btn);
    if (m_start_btn)
    {
        m_start_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_start_clicked));
    }

    m_builder->get_widget("stop_btn", m_stop_btn);
    if (m_stop_btn)
    {
        m_stop_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_stop_clicked));
    }    

    m_builder->get_widget("snap_source_cbox", m_snap_source_cbox);

    m_builder->get_widget("snap_btn", m_snap_btn);
    if (m_snap_btn)
    {
        m_snap_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_snap_clicked));
    }

    m_builder->get_widget("toolkit_drawing_area", m_toolkit_display_area);
    if (m_toolkit_display_area)
    {
        m_toolkit_display_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_display_area_draw));

        // Connect mouse scroll event
        m_toolkit_display_area->add_events(Gdk::SCROLL_MASK);
        m_toolkit_display_area->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_display_area_scroll_event));

        // Connect mouse press and motion events
        m_toolkit_display_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_toolkit_display_area->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_display_area_btn_press_event));
        m_toolkit_display_area->signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_display_area_btn_release_event));
        m_toolkit_display_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_display_area_motion_notify_event));
    }

    m_builder->get_widget("cam_grid", m_cam_grid);

    m_builder->get_widget("settings_drawing_area", m_settings_display_area);
    if (m_settings_display_area)
    {
        m_settings_display_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_settings_display_area_draw));

        // Connect mouse scroll event
        m_settings_display_area->add_events(Gdk::SCROLL_MASK);
        m_settings_display_area->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_settings_display_area_scroll_event));

        // Connect mouse press and motion events
        m_settings_display_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_settings_display_area->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_settings_display_area_btn_press_event));
        m_settings_display_area->signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_settings_display_area_btn_release_event));
        m_settings_display_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_settings_display_area_motion_notify_event));
    }

    m_builder->get_widget("sn_lbl", m_sn_lbl);

    m_builder->get_widget("exposure_time_entry", m_exposure_time_entry);
    if (m_exposure_time_entry)
    {
        m_exposure_time_entry->signal_focus_out_event().connect(sigc::mem_fun(*this, &MainWindow::on_exposure_time_entry_focus_out));      
    }

    m_width_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(m_builder->get_object("width_adjustment"));

    m_height_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(m_builder->get_object("height_adjustment"));

    m_offset_x_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(m_builder->get_object("offset_x_adjustment"));

    m_offset_y_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(m_builder->get_object("offset_y_adjustment"));

    m_debounce_time_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(m_builder->get_object("debounce_time_adjustment"));

    m_builder->get_widget("width_sb", m_width_sb);
    if (m_width_sb)
    {
        m_width_sb->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_width_value_changed));
    }

    m_builder->get_widget("height_sb", m_height_sb);
    if (m_height_sb)
    {
        m_height_sb->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_height_value_changed));
    }

    m_builder->get_widget("offset_x_sb", m_offset_x_sb);
    if (m_offset_x_sb)
    {
        m_offset_x_sb->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_offset_x_value_changed));
    }

    m_builder->get_widget("offset_y_sb", m_offset_y_sb);
    if (m_offset_y_sb)
    {
        m_offset_y_sb->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_offset_y_value_changed));
    }

    m_builder->get_widget("settings_digital_output_line_number_cbox", m_settings_digital_output_line_number_cbox);
    if (m_settings_digital_output_line_number_cbox)
    {
        m_settings_digital_output_line_number_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_digital_output_line_number_changed));
    }

    m_builder->get_widget("settings_digital_output_line_mode_cbox", m_settings_digital_output_line_mode_cbox);
    if (m_settings_digital_output_line_mode_cbox)
    {
        m_settings_digital_output_line_mode_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_digital_output_line_mode_changed));
    }

    m_builder->get_widget("settings_digital_output_line_source_cbox", m_settings_digital_output_line_source_cbox);
    if (m_settings_digital_output_line_source_cbox)
    {
        m_settings_digital_output_line_source_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_digital_output_line_source_changed));
    }

    m_builder->get_widget("settings_strobe_enable_switch", m_settings_strobe_enable_switch);
    if (m_settings_strobe_enable_switch)
    {
        m_settings_strobe_enable_switch->signal_state_set().connect(sigc::mem_fun(*this, &MainWindow::on_strobe_enable_state_set));
    }

    m_builder->get_widget("settings_strobe_duration_sb", m_settings_strobe_duration_sb);
    if (m_settings_strobe_duration_sb)
    {
        m_settings_strobe_duration_sb->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_strobe_duration_value_changed));
    }
    
    m_builder->get_widget("save_camera_settings_btn", m_save_camera_settings_btn);
    if (m_save_camera_settings_btn)
    {
        m_save_camera_settings_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_save_camera_settings_clicked));
    }

    m_builder->get_widget("toolkit_stack", m_toolkit_stack);

    m_builder->get_widget("toolkit_anomaly_detection_rbtn", m_toolkit_anomaly_detection_rbtn);
    if (m_toolkit_anomaly_detection_rbtn)
    {
        m_toolkit_anomaly_detection_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_toggled));
    }

    m_builder->get_widget("toolkit_digital_io_rbtn", m_toolkit_digital_io_rbtn);
    if (m_toolkit_digital_io_rbtn)
    {
        m_toolkit_digital_io_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_toggled));
    }

    m_builder->get_widget("check_service_status_btn", m_check_service_status_btn);
    if (m_check_service_status_btn)
    {
        m_check_service_status_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_check_service_status_clicked));
    }

    m_builder->get_widget("service_status_lbl", m_service_status_lbl);

    m_builder->get_widget("toolkit_digital_output_source_cbox", m_toolkit_digital_output_source_cbox);

    m_builder->get_widget("toolkit_digital_output_line_number_cbox", m_toolkit_digital_output_line_number_cbox);

    m_builder->get_widget("toolkit_strobe_enable_switch", m_toolkit_strobe_enable_switch);

    m_strobe_duration_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(m_builder->get_object("strobe_duration_adjustment"));

    m_builder->get_widget("toolkit_strobe_duration_sb", m_toolkit_strobe_duration_sb);

    m_builder->get_widget("toolkit_test_digital_out_btn", m_toolkit_test_digital_out_btn);
    if (m_toolkit_test_digital_out_btn)
    {
        m_toolkit_test_digital_out_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_test_digital_out_clicked));
    }

    m_builder->get_widget("toolkit_digital_input_sink_cbox", m_toolkit_digital_input_sink_cbox);

    m_builder->get_widget("toolkit_digital_input_line_number_cbox", m_toolkit_digital_input_line_number_cbox);

    m_builder->get_widget("toolkit_digital_input_debounce_time_sb", m_toolkit_digital_input_debounce_time_sb);

    m_builder->get_widget("toolkit_digital_input_event_trigger_cbox", m_toolkit_digital_input_event_trigger_cbox);

    m_builder->get_widget("toolkit_digital_input_notification_status_cbox", m_toolkit_digital_input_notification_status_cbox);

    m_builder->get_widget("start_digital_input_event_listening_btn", m_start_digital_input_event_listening_btn);
    if (m_start_digital_input_event_listening_btn)
    {
        m_start_digital_input_event_listening_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_start_listening_digital_input_event_clicked));
    }

    m_builder->get_widget("stop_digital_input_event_listening_btn", m_stop_digital_input_event_listening_btn);
    if (m_stop_digital_input_event_listening_btn)
    {
        m_stop_digital_input_event_listening_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_stop_listening_digital_input_event_clicked));
    }

    m_builder->get_widget("settings_stack", m_settings_stack);

    m_builder->get_widget("anomaly_detection_settings_rbtn", m_anomaly_detection_settings_rbtn);
    if (m_anomaly_detection_settings_rbtn)
    {
        m_anomaly_detection_settings_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_settings_toggled));
    }

    m_builder->get_widget("camera_settings_rbtn", m_camera_settings_rbtn);
    if (m_camera_settings_rbtn)
    {
        m_camera_settings_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_settings_toggled));
    }

    m_builder->get_widget("select_detection_camera_cbox", m_select_detection_camera_cbox);

    m_builder->get_widget("detection_rate_sb", m_detection_rate_sb);
    
    m_builder->get_widget("select_detection_digital_input_cbox", m_select_detection_digital_input_cbox);
    if (m_select_detection_digital_input_cbox)
    {
        m_select_detection_digital_input_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_detection_digital_input_selection_changed));
    }

    m_builder->get_widget("settings_detection_digital_input_line_number_lbl", m_settings_detection_digital_input_line_number_lbl);

    m_builder->get_widget("select_detection_digital_output_cbox", m_select_detection_digital_output_cbox);
    if (m_select_detection_digital_output_cbox)
    {
        m_select_detection_digital_output_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_detection_digital_output_selection_changed));
    }

    m_builder->get_widget("settings_detection_digital_output_line_number_lbl", m_settings_detection_digital_output_line_number_lbl);

    m_builder->get_widget("detection_sensitivity_scale", m_detection_sensitivity_scale);
    if (m_detection_sensitivity_scale)
    {
        m_detection_sensitivity_scale->signal_format_value().connect([](double value)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << (value * 100) << "%";
            return oss.str(); 
        });
    }

    m_builder->get_widget("anomaly_size_threshold_scale", m_anomaly_size_threshold_scale);
    if (m_anomaly_size_threshold_scale)
    {
        m_anomaly_size_threshold_scale->signal_format_value().connect([](double value)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << (value * 100) << "%";
            return oss.str(); 
        });
    }

    m_builder->get_widget("cancel_detection_settings_btn", m_cancel_detection_settings_btn);
    if (m_cancel_detection_settings_btn)
    {
        m_cancel_detection_settings_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_cancel_detection_settings_clicked));
    }

    m_builder->get_widget("save_detection_settings_btn", m_save_detection_settings_btn);
    if (m_save_detection_settings_btn)
    {
        m_save_detection_settings_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_save_detection_settings_clicked));
    }

    m_builder->get_widget("recent_detection_results_selector_cbox", m_recent_detection_results_selector_cbox);
    if (m_recent_detection_results_selector_cbox)
    {
        m_recent_detection_results_selector_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_recent_detection_results_selector_changed));
    }

    m_builder->get_widget("detection_results_listbox", m_detection_results_listbox);
    if (m_detection_results_listbox)
    {
        m_detection_results_listbox->signal_row_selected().connect(sigc::mem_fun(*this, &MainWindow::on_detection_result_selected));
    }

    m_builder->get_widget("detection_results_display_area", m_detection_results_display_area);
    if (m_detection_results_display_area)
    {
        m_detection_results_display_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_detection_results_display_area_draw));

        // Connect mouse scroll event
        m_detection_results_display_area->add_events(Gdk::SCROLL_MASK);
        m_detection_results_display_area->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_detection_display_area_scroll_event));

        // Connect mouse press and motion events
        m_detection_results_display_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_detection_results_display_area->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_detection_display_area_btn_press_event));
        m_detection_results_display_area->signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_detection_display_area_btn_release_event));
        m_detection_results_display_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_detection_display_area_motion_notify_event));
    }

    m_builder->get_widget("detection_results_refresh_btn", m_detection_results_refresh_btn);
    if (m_detection_results_refresh_btn)
    {
        m_detection_results_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_detection_results_refresh_clicked));
    }

    m_builder->get_widget("last_detection_results_refresh_time", m_last_detection_results_refresh_time_lbl);

    setup_directory_monitor(AppPaths::Detection_Results_Path.string());

    // Set the default time to one second before the app starts
    m_last_load_time = std::chrono::steady_clock::now() - std::chrono::seconds(1);

    m_builder->get_widget("detection_results_path_lbl", m_detection_results_path_lbl);

    m_builder->get_widget("max_per_day_sb", m_max_per_day_sb);
    if (m_max_per_day_sb)
    {
        m_max_per_day_sb->signal_value_changed().connect([this]() {
            auto max_per_day = m_max_per_day_sb->get_value_as_int();
            auto days_to_retain = m_days_to_retain_sb->get_value_as_int();
            update_detection_results_memory_usage_label(max_per_day, days_to_retain);
        });
    }

    m_builder->get_widget("days_to_retain_sb", m_days_to_retain_sb);
    if (m_days_to_retain_sb)
    {
        m_days_to_retain_sb->signal_value_changed().connect([this]() {
            auto max_per_day = m_max_per_day_sb->get_value_as_int();
            auto days_to_retain = m_days_to_retain_sb->get_value_as_int();
            update_detection_results_memory_usage_label(max_per_day, days_to_retain);
        });
    }

    m_builder->get_widget("results_memory_usage_lbl", m_detection_results_memory_usage_lbl);

    m_builder->get_widget("delete_results_btn", m_delete_detection_results_btn);
    if (m_delete_detection_results_btn)
    {
        m_delete_detection_results_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_delete_detection_results_clicked));
    }

    m_builder->get_widget("digital_io_type_cbox", m_digital_io_type_cbox);
    if (m_digital_io_type_cbox)
    {
        m_digital_io_type_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_digital_io_type_changed));
    }

    m_builder->get_widget("digital_input_grid", m_digital_input_grid);

    m_builder->get_widget("detection_digital_input_lbl", m_detection_digital_input_lbl);

    m_builder->get_widget("detection_digital_output_lbl", m_detection_digital_output_lbl);

    m_builder->get_widget("detection_digital_input_line_number_lbl", m_detection_digital_input_line_number_lbl);

    m_builder->get_widget("digital_output_grid", m_digital_output_grid);

    m_builder->get_widget("detection_digital_output_line_number_lbl", m_detection_digital_output_line_number_lbl);

    m_builder->get_widget("settings_digital_input_line_number_cbox", m_settings_digital_input_line_number_cbox);

    m_builder->get_widget("settings_digital_input_debouncer_time_sb", m_settings_digital_input_debouncer_time_sb);

    m_builder->get_widget("settings_digital_input_event_trigger_cbox", m_settings_digital_input_event_trigger_cbox);

    m_builder->get_widget("settings_digital_input_event_status_cbox", m_settings_digital_input_notification_status_cbox);

    m_builder->get_widget("digital_input_event_status_lbl", m_digital_input_event_status_lbl);

    m_builder->get_widget("digital_input_event_tv", m_digital_input_event_tv);
}

void MainWindow::on_detection_digital_input_selection_changed()
{
    std::string digital_input = m_select_detection_digital_input_cbox->get_active_text();
    std::string digital_input_line_number;

    if (digital_input != "")
    {
        auto settings = SettingsService::get_settings("[" + digital_input + "]");
        for (const auto &[key, value] : settings)
        {
            if (key == "digital_input_line_number")
            {
                digital_input_line_number = value;
                break;
            }
        }
    }

    if (m_settings_detection_digital_input_line_number_lbl)
    {
        m_settings_detection_digital_input_line_number_lbl->set_text(digital_input_line_number);
    }    
}

void MainWindow::on_detection_digital_output_selection_changed()
{
    std::string digital_output = m_select_detection_digital_output_cbox->get_active_text();
    std::string digital_output_line_number;

    if (digital_output != "")
    {
        auto settings = SettingsService::get_settings("[" + digital_output + "]");
        for (const auto &[key, value] : settings)
        {
            if (key == "digital_output_line_number")
            {
                digital_output_line_number = value;
                break;
            }
        }
    }

    if (m_settings_detection_digital_output_line_number_lbl)
    {
        m_settings_detection_digital_output_line_number_lbl->set_text(digital_output_line_number);
    }       
}

void MainWindow::on_delete_detection_results_clicked()
{
    Gtk::MessageDialog dialog(*this, "Are you sure you want to delete all files?",
                              false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
    dialog.set_secondary_text("This action cannot be undone.");

    // Show the dialog and get user response
    int response = dialog.run();

    if (response == Gtk::RESPONSE_YES)
    {
        FileUtils::delete_all_in_directory(AppPaths::Detection_Results_Path); // Replace with your directory path
    }
    else
    {
        std::cout << "Deletion canceled by the user." << std::endl;
    }
}

void MainWindow::setup_directory_monitor(const std::string &directory_path)
{
    auto directory = Gio::File::create_for_path(directory_path);

    // Create a file monitor
    m_detection_results_monitor = directory->monitor_directory();

    // Connect the signal
    m_detection_results_monitor->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_directory_changed));
}

void MainWindow::on_directory_changed(
    const Glib::RefPtr<Gio::File> &file,
    const Glib::RefPtr<Gio::File> &other_file,
    Gio::FileMonitorEvent event_type)
{
    // React to the change
    if (event_type == Gio::FILE_MONITOR_EVENT_CREATED ||
        event_type == Gio::FILE_MONITOR_EVENT_CHANGED)
    {
        auto now = std::chrono::steady_clock::now();
        // Throttling mechanism
        // if (std::chrono::duration_cast<std::chrono::seconds>(now - m_last_load_time).count() > 1)
        // {
        //     m_last_load_time = now;
        //     load_detection_results();
        // }

        // Wait a bit for the detection results ready for loading
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        m_last_load_time = now;
        load_detection_results();
    }
}

void MainWindow::on_recent_detection_results_selector_changed()
{
    load_detection_results();
}

void MainWindow::on_detection_results_refresh_clicked()
{
    m_last_load_time = std::chrono::steady_clock::now();
    load_detection_results();
}

void MainWindow::load_detection_results()
{
    // Clear the resutls before loading
    for (auto *child : m_detection_results_listbox->get_children())
    {
        m_detection_results_listbox->remove(*child);
    }

    if (m_last_detection_results_refresh_time_lbl)
    {
        // Convert m_last_load_time to a time_t (assuming steady_clock is close to system_clock)
        auto now_c = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() + (m_last_load_time - std::chrono::steady_clock::now()));

        // Format time as a string
        std::ostringstream time_stream;
        time_stream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

        m_last_detection_results_refresh_time_lbl->set_text(time_stream.str());
    }

    if (m_recent_detection_results_selector_cbox)
    {
        auto result_count = std::stoi(m_recent_detection_results_selector_cbox->get_active_id());
        const char* home = std::getenv("HOME");
        auto recent_results_folders = FileUtils::get_recent_folders(AppPaths::Detection_Results_Path, result_count);
        
        // Populating the detection results list box with rows
        for (const auto &result_folder : recent_results_folders)
        {
            std::cout << result_folder << std::endl;

            auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
            auto trans_label = Gtk::make_managed<Gtk::Label>(result_folder.filename().string());
            row_box->set_tooltip_text(result_folder.string());
            row_box->pack_start(*trans_label, Gtk::PACK_SHRINK);
            // Create a Gtk::ListBoxRow to wrap the box
            auto listbox_row = Gtk::make_managed<Gtk::ListBoxRow>();
            listbox_row->add(*row_box);
            // Set margin around the row
            listbox_row->set_margin_top(5);      // Space above the row
            listbox_row->set_margin_bottom(5);   // Space below the row
            listbox_row->set_margin_start(5);   // Space to the left of the row
            listbox_row->set_margin_end(5);     // Space to the right of the row
            // Add the Gtk::ListBoxRow to the list box
            m_detection_results_listbox->append(*listbox_row);
            // Show all the newly added widgets
            listbox_row->show_all();
        }

        // Select the first row
        auto most_recent_result = m_detection_results_listbox->get_row_at_index(0);
        if (most_recent_result)
        {
            m_detection_results_listbox->select_row(*most_recent_result);
        }
    }
}

void MainWindow::on_detection_result_selected(Gtk::ListBoxRow* row)
{
    if (row)
    {
        auto row_box = dynamic_cast<Gtk::Box*>(row->get_child());
        if (row_box)
        {
            std::string result_folder = row_box->get_tooltip_text();
            std::cout << "Selected row: " << result_folder << std::endl;
            load_detection_result(result_folder);
        }
    }
    else
    {
        std::cout << "No row selected!" << std::endl;
    }
}

void MainWindow::load_detection_result(std::string &detection_result_folder)
{
    std::filesystem::path trans_json = std::filesystem::path(detection_result_folder) / "transaction_data.json";
    if (!std::filesystem::exists(trans_json))
    {
        std::cerr << "File not exists: transaction_data.json" << std::endl;
        return;
    }

    // Read the content of the JSON file
    std::ifstream json_file(trans_json);
    if (!json_file.is_open())
    {
        std::cerr << "Failed to open the file." << std::endl;
        return;
    }

    // Parse the JSON content
    nlohmann::json json_data;
    json_file >> json_data;

    int patch_size = json_data["patch_size"].get<int>();
    int frame_width = json_data["frame_width"].get<int>();
    int frame_height = json_data["frame_height"].get<int>();
    int num_frames = json_data["num_frames"].get<int>();
    int total_height = frame_height * num_frames;

    // Create the combined pixbuf for detection images.
    // Gdk::Pixbuf does not directly support a single-channel format, 
    // so still create an RGB pixbuf and replicate the grayscale values across the three color channels.
    m_image_pixbuf_detection_result = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    m_image_pixbuf_detection_result->fill(0x00000000); // Fill with black

    int current_y = 0;
    bool load_images_error = false;

    // Load and position each image
    for (const auto &frame : json_data["frames"])
    {
        const std::string& path = frame["file_name"];
        auto pixbuf_image = Gdk::Pixbuf::create_from_file(path, frame_width, frame_height);
        if (!pixbuf_image)
        {
            load_images_error = true;
            std::cerr << "Failed to load image(s): " << path << std::endl;
            break;
        }

        // Copy the current image into the combined pixbuf
        pixbuf_image->copy_area(
            0, 
            0, 
            frame_width, 
            frame_height, 
            m_image_pixbuf_detection_result, 
            0, 
            current_y);

        // Update the y-offset for the next image
        current_y += frame_height;
    }

    if (load_images_error)
    {
        // TODO, show a popup
        return;
    }

    // Create a transparent mask pixbuf of the same size as the image
    m_mask_pixbuf_detection_result = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    // m_mask_pixbuf_detection_result->fill(0xffffffbe); // For testing
    m_mask_pixbuf_detection_result->fill(0x00000000); // Initialize the mask to be fully transparent black

    // Load and position each prediction
    int predictions_per_row = frame_width / patch_size;
    if (frame_width % patch_size != 0)
    {
        predictions_per_row++; // Allow for an additional prediction if there's remaining space
    }
    for (const auto &prediction : json_data["predictions"])
    {
        int prediction_id = prediction["prediction_id"].get<int>();
        std::string filename = prediction["file_name"].get<std::string>();
        
        // Load the prediction image
        auto prediction_pixbuf = Gdk::Pixbuf::create_from_file(filename);
        if (!prediction_pixbuf)
        {
            std::cerr << "Failed to load prediction image" << std::endl;
            continue;
        }

        // Calculate row and column based on the index
        int row = prediction_id / predictions_per_row;
        int col = prediction_id % predictions_per_row;

        // Calculate position_x
        int position_x = col * patch_size; // Standard position in the row

        // Adjust position_x if this is the last column and it exceeds frame width
        if (col == predictions_per_row - 1 && position_x + patch_size > frame_width)
        {
            position_x = frame_width - patch_size;
        }

        // Calculate position_y
        int position_y = row * patch_size; // Each row is separated by the height of the patch

        // Copy the prediction image into m_mask_pixbuf_toolkit at the specified position
        prediction_pixbuf->Gdk::Pixbuf::copy_area(
            0,
            0,
            prediction_pixbuf->get_width(),
            prediction_pixbuf->get_height(),
            m_mask_pixbuf_detection_result,
            position_x,
            position_y
        );
    }

    // Update mask pixel buf
    if (m_mask_pixbuf_detection_result)
    {
        update_mask_color(m_mask_pixbuf_detection_result);
        update_mask_alpha(m_mask_pixbuf_detection_result, m_mask_alpha * 255);
    }

    // Queue the frame for display
    if (m_image_pixbuf_detection_result)
    {
        m_detection_results_display_area->set_size_request(frame_width, total_height);
        m_detection_results_display_area->queue_draw();
    }
}

bool MainWindow::on_detection_results_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x_detection, m_offset_y_detection);   // Apply panning offset
    cr->scale(m_zoom_factor_detection, m_zoom_factor_detection); // Apply zoom

    // Draw the images
    if (m_image_pixbuf_detection_result)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_detection_result, 0, 0);
        cr->paint();
    }

    // Draw the masks
    if (m_mask_pixbuf_detection_result)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_mask_pixbuf_detection_result, 0, 0);
        cr->paint();
    }

    return true;
}

void MainWindow::load_detection_settings()
{
    std::string detection_camera;
    auto detection_rate = 0;
    std::string digital_input;
    std::string digital_input_line_number;
    std::string digital_output;
    std::string digital_output_line_number;
    auto confidence_threshold = 0.5;
    auto pixel_threshold = 0.1;
    auto max_per_day = 1000;
    auto days_to_retain = 30;

    auto settings = SettingsService::get_settings("[detection]");
    for (const auto &[key, value] : settings)
    {
        if (key == "detection_camera")
        {
            detection_camera = value;
        }
        else if (key == "detection_rate")
        {
            detection_rate = std::stod(value);
        }
        else if (key == "digital_input")
        {
            digital_input = value;
        }
        else if (key == "digital_output")
        {
            digital_output = value;
        }
        else if (key == "confidence_threshold")
        {
            confidence_threshold = std::stod(value);
        }
        else if (key == "pixel_threshold")
        {
            pixel_threshold = std::stod(value);
        }
        else if (key == "max_per_day")
        {
            max_per_day = std::stod(value);
        }
        else if (key == "days_to_retain")
        {
            days_to_retain = std::stod(value);
        }
    }

    if (digital_input != "")
    {
        auto settings = SettingsService::get_settings("[" + digital_input + "]");
        for (const auto &[key, value] : settings)
        {
            if (key == "digital_input_line_number")
            {
                digital_input_line_number = value;
                break;
            }
        }
    }

    if (digital_output != "")
    {
        auto settings = SettingsService::get_settings("[" + digital_output + "]");
        for (const auto &[key, value] : settings)
        {
            if (key == "digital_output_line_number")
            {
                digital_output_line_number = value;
                break;
            }
        }
    }

    if (m_detection_camera_lbl)
    {
        m_detection_camera_lbl->set_text(detection_camera);
    }
    if (m_select_detection_camera_cbox)
    {
        m_select_detection_camera_cbox->set_active_text(detection_camera);
    }
    if (m_detection_rate_lbl)
    {
        m_detection_rate_lbl->set_text(std::to_string(detection_rate));
    }
    if (m_detection_rate_sb)
    {
        m_detection_rate_sb->set_value(detection_rate);
    }
    if (m_detection_digital_input_lbl)
    {
        m_detection_digital_input_lbl->set_text(digital_input);
    }
    if (m_detection_digital_input_line_number_lbl)
    {
        m_detection_digital_input_line_number_lbl->set_text(digital_input_line_number);
    }
    if (m_select_detection_digital_input_cbox)
    {
        m_select_detection_digital_input_cbox->set_active_text(digital_input);
    }
    if (m_settings_detection_digital_input_line_number_lbl)
    {
        m_settings_detection_digital_input_line_number_lbl->set_text(digital_input_line_number);
    }
    if (m_detection_digital_output_lbl)
    {
        m_detection_digital_output_lbl->set_text(digital_output);
    }
    if (m_detection_digital_output_line_number_lbl)
    {
        m_detection_digital_output_line_number_lbl->set_text(digital_output_line_number);
    }
    if (m_select_detection_digital_output_cbox)
    {
        m_select_detection_digital_output_cbox->set_active_text(digital_output);
    }
    if (m_settings_detection_digital_output_line_number_lbl)
    {
        m_settings_detection_digital_output_line_number_lbl->set_text(digital_output_line_number);
    }
    if (m_detection_sensitivity_scale)
    {
        m_detection_sensitivity_scale->set_value(confidence_threshold);
    }
    if (m_anomaly_size_threshold_scale)
    {
        m_anomaly_size_threshold_scale->set_value(pixel_threshold);
    }
    if (m_detection_results_path_lbl)
    {
        m_detection_results_path_lbl->set_text(AppPaths::Detection_Results_Path.string());
    }
    if (m_max_per_day_sb)
    {
        m_max_per_day_sb->set_value(max_per_day);
    }
    if (m_days_to_retain_sb)
    {
        m_days_to_retain_sb->set_value(days_to_retain);
    }
    update_detection_results_memory_usage_label(max_per_day, days_to_retain);
}

void MainWindow::update_detection_results_memory_usage_label(size_t max_per_day, size_t days_to_retain)
{
    if (m_detection_results_memory_usage_lbl)
    {
        auto memory_usage_gb = calc_detection_results_memory_usage_in_gb(max_per_day, days_to_retain);

        // Format the memory usage with 2 decimal places
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << memory_usage_gb << " GB";

        // Set the formatted string to the label
        m_detection_results_memory_usage_lbl->set_text(oss.str());
    }
}

double MainWindow::calc_detection_results_memory_usage_in_gb(size_t max_per_day, size_t days_to_retain)
{
    constexpr size_t file_size_kb = 400;      // Size of one file in KB
    constexpr double kb_to_gb = 1.0 / 1048576.0; // Conversion factor from KB to GB
    size_t total_kb = max_per_day * days_to_retain * file_size_kb; // Total memory in KB
    return total_kb * kb_to_gb;
}

void MainWindow::on_cancel_detection_settings_clicked()
{
    load_detection_settings();
}

void MainWindow::on_save_detection_settings_clicked()
{
    // Create settings header
    std::string settings_header = "[detection]";

    // Build settings content
    std::stringstream settings_content;
    settings_content << settings_header << std::endl;
    if (m_select_detection_camera_cbox)
    {
        auto detection_camera = m_select_detection_camera_cbox->get_active_text();
        settings_content << "detection_camera=" << detection_camera << std::endl;
        m_detection_camera_lbl->set_text(detection_camera);
    }
    if (m_detection_rate_sb)
    {
        auto detection_rate = m_detection_rate_sb->get_value_as_int();
        settings_content << "detection_rate=" << detection_rate << std::endl;
        m_detection_rate_lbl->set_text(std::to_string(detection_rate));
    }
    if (m_select_detection_digital_input_cbox)
    {
        auto digital_input = m_select_detection_digital_input_cbox->get_active_text();
        settings_content << "digital_input=" << digital_input << std::endl;
    }
    if (m_select_detection_digital_output_cbox)
    {
        auto digital_output = m_select_detection_digital_output_cbox->get_active_text();
        settings_content << "digital_output=" << digital_output << std::endl;
    }
    if (m_detection_sensitivity_scale)
    {
        auto confidence_threshold = m_detection_sensitivity_scale->get_value();
        settings_content << "confidence_threshold=" << confidence_threshold << std::endl;
    }
    if (m_anomaly_size_threshold_scale)
    {
        auto pixel_threshold = m_anomaly_size_threshold_scale->get_value();
        settings_content << "pixel_threshold=" << pixel_threshold << std::endl;
    }
    if (m_max_per_day_sb)
    {
        auto max_per_day = m_max_per_day_sb->get_value_as_int();
        settings_content << "max_per_day=" << max_per_day << std::endl;
    }
    if (m_days_to_retain_sb)
    {
        auto days_to_retain = m_days_to_retain_sb->get_value_as_int();
        settings_content << "days_to_retain=" << days_to_retain << std::endl;
    }
    settings_content << std::endl; // Add a blank line after the new section

    // Convert to a normal string
    std::string settings_string = settings_content.str();

    // Save detection settings
    SettingsService::save_settings(settings_string, settings_header);
}

void MainWindow::on_digital_io_type_changed()
{
    std::string type = m_digital_io_type_cbox->get_active_text();
    if (type == "Input")
    {
        m_digital_input_grid->set_visible(true);
        m_digital_output_grid->set_visible(false);
    }
    else if (type == "Output")
    {
        m_digital_input_grid->set_visible(false);
        m_digital_output_grid->set_visible(true);
    }
    else
    {
        m_digital_input_grid->set_visible(false);
        m_digital_output_grid->set_visible(false);        
    }
}

void MainWindow::on_digital_output_line_number_changed()
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        std::string selected_line_number = m_settings_digital_output_line_number_cbox->get_active_text();

        if (selected_line_number != "")
        {
            int nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", selected_line_number.c_str());
            if (nRet == MV_OK)
            {
                // Wait a bit or Network error occurs - MV_E_NETER (0x80000206)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Update LineMode
                m_settings_digital_output_line_mode_cbox->remove_all();

                MVCC_ENUMVALUE line_mode = {0};
                nRet = MV_CC_GetEnumValue(device_handle, "LineMode", &line_mode);
                if (nRet == MV_OK)
                {
                    std::string active_text = "";
                    for (unsigned int i = 0; i < line_mode.nSupportedNum; ++i)
                    {
                        MVCC_ENUMENTRY line_entry = {0};
                        line_entry.nValue = line_mode.nSupportValue[i];
                        nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "LineMode", &line_entry);
                        if (nRet == MV_OK)
                        {
                            m_settings_digital_output_line_mode_cbox->append(line_entry.chSymbolic);
                            if (line_entry.nValue == line_mode.nCurValue)
                            {
                                active_text = line_entry.chSymbolic;
                            }
                        }
                        else
                        {
                            std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
                        }
                    }
                }
                else
                {
                    std::cerr << "Failed to get line_mode. Error code: " << nRet << std::endl;
                }
            }
            else
            {
                std::cerr << "Failed to set line_number. Error code: " << nRet << std::endl;
            }
        }
    }
}

void MainWindow::on_digital_output_line_mode_changed()
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        std::string selected_line_mode = m_settings_digital_output_line_mode_cbox->get_active_text();

        if (selected_line_mode != "")
        {
            int nRet = MV_CC_SetEnumValueByString(device_handle, "LineMode", selected_line_mode.c_str());
            if (nRet == MV_OK)
            {
                // Wait a bit or Network error occurs - MV_E_NETER (0x80000206)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Update Line Source
                m_settings_digital_output_line_source_cbox->remove_all();

                MVCC_ENUMVALUE line_source = {0};
                nRet = MV_CC_GetEnumValue(device_handle, "LineSource", &line_source);
                if (nRet == MV_OK)
                {
                    std::string active_text = "";
                    for (unsigned int i = 0; i < line_source.nSupportedNum; ++i)
                    {
                        MVCC_ENUMENTRY line_entry = {0};
                        line_entry.nValue = line_source.nSupportValue[i];
                        nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "LineSource", &line_entry);
                        if (nRet == MV_OK)
                        {
                            m_settings_digital_output_line_source_cbox->append(line_entry.chSymbolic);
                            if (line_entry.nValue == line_source.nCurValue)
                            {
                                active_text = line_entry.chSymbolic;
                            }
                        }
                        else
                        {
                            std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
                        }
                    }
                }
                else
                {
                    std::cerr << "Failed to get line_source. Error code: " << nRet << std::endl;
                }
            }
            else
            {
                std::cerr << "Failed to set line_mode to " << selected_line_mode << " Error code: " << nRet << std::endl;
            }
        }
    }
}

void MainWindow::on_digital_output_line_source_changed()
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        std::string selected_line_source = m_settings_digital_output_line_source_cbox->get_active_text();

        if (selected_line_source != "")
        {
            int nRet = MV_CC_SetEnumValueByString(device_handle, "LineSource", selected_line_source.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set LineSource to SoftTriggerActive. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValue(LineSource) to SoftTriggerActive: " + std::to_string(nRet), Logger::ERROR);
            }
        }
    }
}

bool MainWindow::on_strobe_enable_state_set(bool state)
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        int nRet = MV_CC_SetBoolValue(device_handle, "StrobeEnable", state);
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set StrobeEnable to " << state << " Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetBoolValue(StrobeEnable). Error code: " + std::to_string(nRet), Logger::ERROR);
        }
    }

    return false; // Returning false allows the default handler to run
}

void MainWindow::on_strobe_duration_value_changed()
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        double strobe_duration = m_settings_strobe_duration_sb->get_value();
        int nRet = MV_CC_SetIntValue(device_handle, "StrobeLineDuration", static_cast<int>(strobe_duration));
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set StrobeLineDuration to " << static_cast<int>(strobe_duration) << " Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(StrobeLineDuration). Error code: " + std::to_string(nRet), Logger::ERROR);
        }
    }
}

void MainWindow::on_test_digital_out_clicked()
{
    auto sn = m_toolkit_digital_output_source_cbox->get_active_text();
    auto line_number = m_toolkit_digital_output_line_number_cbox->get_active_text();
    bool strobe_enable = m_toolkit_strobe_enable_switch->get_active();
    double strobe_duration = m_toolkit_strobe_duration_sb->get_value();

    if (!connect_camera(sn))
    {
        std::cout << "Failed to connect to the camera: " << sn << std::endl;
        m_logger->log("Failed to connect to the camera: " + sn, Logger::ERROR);
        return;
    }

    auto device_handle = create_or_get_device_handle_by_serial_number(sn);

    int nRet = MV_CC_SetEnumValue(device_handle, "LineSelector", std::stoi(line_number));
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set LineSelector. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(LineSelector): " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    nRet = MV_CC_SetEnumValue(device_handle, "LineMode", 8); // 8:Strobe
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set LineMode to Strobe. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(LineMode) to Strobe: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    nRet = MV_CC_SetEnumValue(device_handle, "LineSource", 5); // 5:SoftTriggerActive
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set LineSource to SoftTriggerActive. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(LineSource) to SoftTriggerActive: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    nRet = MV_CC_SetBoolValue(device_handle, "StrobeEnable", strobe_enable);
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set StrobeEnable to " << strobe_enable << " Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetBoolValue(StrobeEnable). Error code: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    nRet = MV_CC_SetIntValue(device_handle, "StrobeLineDuration", static_cast<int>(strobe_duration));
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set StrobeLineDuration to " << static_cast<int>(strobe_duration) << " Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetIntValue(StrobeLineDuration). Error code: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    nRet = MV_CC_SetCommandValue(device_handle, "LineTriggerSoftware");
    if (nRet != MV_OK)
    {
        std::cerr << "Error to send command LineTriggerSoftware. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetCommandValue(LineTriggerSoftware). Error code: " + std::to_string(nRet), Logger::ERROR);
    }
    else
    {
        std::cout << "Trigger digital output via software succeeded" << std::endl;
    }

    if (!disconnect_camera(sn))
    {
        std::cout << "Failed to disconnect from the camera: " << sn << std::endl;
        m_logger->log("Failed to disconnect from the camera: " + sn, Logger::ERROR);
    }
}

void MainWindow::on_start_listening_digital_input_event_clicked()
{
    if (m_is_listening_digital_input_event)
    {
        std::cout << "Already listening to events." << std::endl;
        return;
    }

    m_toolkit_digital_input_sink_cbox->set_sensitive(false);
    m_start_digital_input_event_listening_btn->set_sensitive(false);
    m_digital_input_event_status_lbl->get_style_context()->remove_class("gray-indicator");
    m_digital_input_event_status_lbl->get_style_context()->add_class("green-indicator");
    // Clears the text
    auto buffer = m_digital_input_event_tv->get_buffer();
    buffer->set_text("");

    auto sn = m_toolkit_digital_input_sink_cbox->get_active_text();
    auto line_number = m_toolkit_digital_input_line_number_cbox->get_active_text();
    auto debounce_time = m_toolkit_digital_input_debounce_time_sb->get_value_as_int();
    auto event_trigger = m_toolkit_digital_input_event_trigger_cbox->get_active_text();
    auto notification_status = m_toolkit_digital_input_notification_status_cbox->get_active_text();

    if (!connect_camera(sn))
    {
        std::cout << "Failed to connect to the camera: " << sn << std::endl;
        m_logger->log("Failed to connect to the camera: " + sn, Logger::ERROR);
        return;
    }
    auto device_handle = create_or_get_device_handle_by_serial_number(sn);

    int nRet = MV_CC_SetEnumValue(device_handle, "LineSelector", std::stoi(line_number));
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set LineSelector. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(LineSelector): " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    nRet = MV_CC_SetIntValue(device_handle, "LineDebouncerTime", debounce_time);
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set LineDebouncerTime. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetIntValue(LineDebouncerTime): " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    nRet = MV_CC_SetEnumValueByString(device_handle, "EventSelector", event_trigger.c_str());
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set EventSelector. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValueByString(EventSelector): " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    nRet = MV_CC_SetEnumValueByString(device_handle, "EventNotification", notification_status.c_str());
    if (nRet != MV_OK)
    {
        std::cerr << "Error to set EventNotification. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValueByString(EventNotification): " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    auto event_callback = [](MV_EVENT_OUT_INFO * pEventInfo, void* pUser)
    {
        if (pEventInfo)
        {
            int64_t nBlockId = pEventInfo->nBlockIdHigh;
            nBlockId = (nBlockId << 32) + pEventInfo->nBlockIdLow;

            int64_t nTimestamp = pEventInfo->nTimestampHigh;
            nTimestamp = (nTimestamp << 32) + pEventInfo->nTimestampLow;

            std::ostringstream oss;
            oss << "Timestamp: " << nTimestamp 
                << ", Event Name: " << pEventInfo->EventName 
                << ", Event Id: " << pEventInfo->nEventID 
                << ", Block Id: " << nBlockId;
            std::string eventInfoStr = oss.str();

            // Update the TextView on the main thread
            MainWindow *pThis = static_cast<MainWindow*>(pUser);
            Glib::signal_idle().connect_once([pThis, eventInfoStr]()
            {
                auto buffer = pThis->m_digital_input_event_tv->get_buffer();
                buffer->insert(buffer->end(), eventInfoStr + "\n");
            });

            std::cout << eventInfoStr << std::endl;
            pThis->m_logger->log(eventInfoStr);
        }
    };

    nRet = MV_CC_RegisterEventCallBackEx(device_handle, event_trigger.c_str(), event_callback, this);
    if (nRet != MV_OK)
    {
        std::cerr << "Error to register EventCallBackEx. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_RegisterEventCallBackEx: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    m_is_listening_digital_input_event = true;
    std::cout << "Digital input event listening started." << std::endl;
}

void MainWindow::on_stop_listening_digital_input_event_clicked()
{
    if (!m_is_listening_digital_input_event)
    {
        std::cout << "Not currently listening to events." << std::endl;
        return;
    }

    m_toolkit_digital_input_sink_cbox->set_sensitive(true);
    m_start_digital_input_event_listening_btn->set_sensitive(true);
    m_digital_input_event_status_lbl->get_style_context()->remove_class("green-indicator");
    m_digital_input_event_status_lbl->get_style_context()->add_class("gray-indicator");

    auto sn = m_toolkit_digital_input_sink_cbox->get_active_text();
    if (!disconnect_camera(sn))
    {
        std::cout << "Failed to disconnect from the camera: " << sn << std::endl;
        m_logger->log("Failed to disconnect from the camera: " + sn, Logger::ERROR);
    }

    m_is_listening_digital_input_event = false;
    std::cout << "Digital input event listening stopped." << std::endl;
}

std::string MainWindow::run_command(const std::string& command)
{
    std::array<char, 128> buffer;
    std::string result = "";

    // Open a pipe to the command.
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    // Read the output of the command.
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    return result;
}

void MainWindow::on_check_service_status_clicked()
{
    std::string command = "systemctl status eagle_eye_detection.service";

    try
    {
        std::string output = run_command(command);
        std::cout << "Command Output:\n" << output << std::endl;

        // Regular expression to match the status after 'Active:' (captures any status)
        std::regex status_regex(R"(Active:\s*(\S.*))");
        std::smatch matches;
        std::string status = "status unknown";
        
        if (std::regex_search(output, matches, status_regex))
        {
            status = matches[1];
        }

        std::cout << status << std::endl;
        m_service_status_lbl->set_text(status);
        
        // Remove previous style classes
        m_service_status_lbl->get_style_context()->remove_class("green-text");
        m_service_status_lbl->get_style_context()->remove_class("red-text");

        // Apply new style based on status
        if (status.find("active (running)") != std::string::npos)
        {
            m_service_status_lbl->get_style_context()->add_class("green-text");
        }
        else if (status.find("inactive (dead)") != std::string::npos)
        {
            m_service_status_lbl->get_style_context()->add_class("red-text");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error executing command: " << e.what() << std::endl;
    }
}

void MainWindow::on_save_camera_settings_clicked()
{
    // Create settings header
    std::string serialNumber;
    if (m_sn_lbl)
    {
        serialNumber = m_sn_lbl->get_text();
    }
    std::string settings_header = "[" + serialNumber + "]";

    // Build settings content
    std::stringstream settings_content;
    settings_content << settings_header << std::endl;
    if (m_exposure_time_entry)
    {
        settings_content << "exposure_time=" << m_exposure_time_entry->get_text() << std::endl;
    }
    if (m_width_sb)
    {
        settings_content << "width=" << m_width_sb->get_value() << std::endl;
    }
    if (m_height_sb)
    {
        settings_content << "height=" << m_height_sb->get_value() << std::endl;
    }
    if (m_offset_x_sb)
    {
        settings_content << "offset_x=" << m_offset_x_sb->get_value() << std::endl;
    }
    if (m_offset_y_sb)
    {
        settings_content << "offset_y=" << m_offset_y_sb->get_value() << std::endl;
    }
    if (m_digital_io_type_cbox)
    {
        std::string digital_io_type = m_digital_io_type_cbox->get_active_text();
        if (digital_io_type == "Input")
        {
            if (m_settings_digital_input_line_number_cbox)
            {
                settings_content << "digital_input_line_number=" << m_settings_digital_input_line_number_cbox->get_active_text() << std::endl;
            }
            if (m_settings_digital_input_debouncer_time_sb)
            {
                settings_content << "digital_input_debouncer_time=" << m_settings_digital_input_debouncer_time_sb->get_value() << std::endl;
            }
            if (m_settings_digital_input_event_trigger_cbox)
            {
                settings_content << "digital_input_event_trigger=" << m_settings_digital_input_event_trigger_cbox->get_active_text() << std::endl;
            }
            if (m_settings_digital_input_notification_status_cbox)
            {
                settings_content << "digital_input_notification_status=" << m_settings_digital_input_notification_status_cbox->get_active_text() << std::endl;
            }
        }
        else if (digital_io_type == "Output")
        {
            if (m_settings_digital_output_line_number_cbox)
            {
                settings_content << "digital_output_line_number=" << m_settings_digital_output_line_number_cbox->get_active_text() << std::endl;
            }
            if (m_settings_digital_output_line_mode_cbox)
            {
                settings_content << "digital_output_line_mode=" << m_settings_digital_output_line_mode_cbox->get_active_text() << std::endl;
            }
            if (m_settings_digital_output_line_source_cbox)
            {
                settings_content << "digital_output_line_source=" << m_settings_digital_output_line_source_cbox->get_active_text() << std::endl;
            }
            if (m_settings_strobe_enable_switch)
            {
                settings_content << "digital_output_strobe_enable=" << m_settings_strobe_enable_switch->get_active() << std::endl;
            }
            if (m_settings_strobe_duration_sb)
            {
                settings_content << "digital_output_strobe_duration=" << m_settings_strobe_duration_sb->get_value() << std::endl;
            }
        }
    }

    settings_content << std::endl; // Add a blank line after the new section

    // Convert to a normal string
    std::string settings_string = settings_content.str();

    // Save detection settings
    SettingsService::save_settings(settings_string, settings_header);
}

void MainWindow::snap_and_display(void *device_handle)
{
    int nRet = MV_CC_StartGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StartGrabbing: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Capture one frame
    nRet = MV_CC_SetCommandValue(device_handle, "TriggerSoftware");
    if(nRet != MV_OK)
    {
        std::cout << "Error on TriggerSoftware: " << nRet << std::endl;
        m_logger->log("Error on TriggerSoftware: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Grab one frame from camera
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    nRet = MV_CC_GetIntValue(device_handle, "PayloadSize", &stParam);
    if (nRet != MV_OK)
    {
        std::cout << "Get PayloadSize fail! Error code: " << nRet << std::endl;
        return;
    }

    MV_FRAME_OUT_INFO_EX stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    unsigned char *pData = (unsigned char *)malloc(sizeof(unsigned char) * stParam.nCurValue);
    if (pData == nullptr)
    {
        return;
    }

    unsigned int nDataSize = stParam.nCurValue;
    nRet = MV_CC_GetOneFrameTimeout(device_handle, pData, nDataSize, &stImageInfo, 1000);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_GetOneFrameTimeout fail. Error code: " << nRet << std::endl;
        return;
    }

    auto frame_width = stImageInfo.nWidth;
    auto frame_height = stImageInfo.nHeight;

    // Save the frame to file
    save_tmp_image(pData, stImageInfo, device_handle);

    // Reset image pixel buffer
    if (m_image_pixbuf_settings)
    {
        m_image_pixbuf_settings.reset();
    }

    // Load the frame from file    
    try
    {
        m_image_pixbuf_settings = Gdk::Pixbuf::create_from_file("/tmp/eagle_eye/tmp.jpeg");
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "File Error: " << ex.what() << std::endl;
    }
    catch (const Gdk::PixbufError &ex)
    {
        std::cerr << "Pixbuf Error: " << ex.what() << std::endl;
    }

    // Reset zoom and pan when a new image is loaded
    m_zoom_factor_settings = 1.0;
    m_offset_x_settings = 0.0;
    m_offset_y_settings = 0.0;

    // Queue the frame for diaplay
    if (m_image_pixbuf_settings)
    {
        m_settings_display_area->set_size_request(frame_width, frame_height);
        m_settings_display_area->queue_draw();
    }

    // Stop grabbing images
    nRet = MV_CC_StopGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
    }
}

bool MainWindow::on_exposure_time_entry_focus_out(GdkEventFocus* event)
{
    auto sn = m_sn_lbl->get_text();

    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        std::string new_exposure_time = m_exposure_time_entry->get_text();
        int nRet = MV_CC_SetFloatValue(device_handle, "ExposureTime", std::stof(new_exposure_time));
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set exposure time. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetFloatValue(ExposureTime): " + std::to_string(nRet), Logger::ERROR);
        }
        else
        {
            snap_and_display(device_handle);
        }
    }

    // Return false to allow further processing of the event
    return false;
}

void MainWindow::on_width_value_changed()
{
    auto sn = m_sn_lbl->get_text();

    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        double new_width = m_width_sb->get_value();
        int nRet = MV_CC_SetIntValue(device_handle, "Width", static_cast<int>(new_width));
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set width. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(Width): " + std::to_string(nRet), Logger::ERROR);
        }
        else
        {
            // Update offset_x adjustment
            MVCC_INTVALUE offset_x = {0};
            nRet = MV_CC_GetIntValue(device_handle, "OffsetX", &offset_x);
            if (nRet == MV_OK && m_offset_x_adj)
            {
                m_offset_x_adj->set_lower(offset_x.nMin);
                m_offset_x_adj->set_upper(offset_x.nMax);
            }

            snap_and_display(device_handle);
        }
    }
}

void MainWindow::on_height_value_changed()
{
    auto sn = m_sn_lbl->get_text();

    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        double new_height = m_height_sb->get_value();
        int nRet = MV_CC_SetIntValue(device_handle, "Height", static_cast<int>(new_height));
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set height. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(Height): " + std::to_string(nRet), Logger::ERROR);
        }
        else
        {
            // Update offset_y adjustment
            MVCC_INTVALUE offset_y = {0};
            nRet = MV_CC_GetIntValue(device_handle, "OffsetY", &offset_y);
            if (nRet == MV_OK && m_offset_y_adj)
            {
                m_offset_y_adj->set_lower(offset_y.nMin);
                m_offset_y_adj->set_upper(offset_y.nMax);
            }

            snap_and_display(device_handle);
        }
    }
}

void MainWindow::on_offset_x_value_changed()
{
    auto sn = m_sn_lbl->get_text();

    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        double new_offset_x = m_offset_x_sb->get_value();
        int nRet = MV_CC_SetIntValue(device_handle, "OffsetX", static_cast<int>(new_offset_x));
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set offsetX. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(OffsetX): " + std::to_string(nRet), Logger::ERROR);
        }
        else
        {
            // Update width adjustment
            MVCC_INTVALUE width = {0};
            nRet = MV_CC_GetIntValue(device_handle, "Width", &width);
            if (nRet == MV_OK && m_width_adj)
            {
                m_width_adj->set_lower(width.nMin);
                m_width_adj->set_upper(width.nMax);
            }

            snap_and_display(device_handle);
        }
    }
}

void MainWindow::on_offset_y_value_changed()
{
    auto sn = m_sn_lbl->get_text();

    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        double new_offset_y = m_offset_y_sb->get_value();
        int nRet = MV_CC_SetIntValue(device_handle, "OffsetY", static_cast<int>(new_offset_y));
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set offsetY. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(OffsetY): " + std::to_string(nRet), Logger::ERROR);
        }
        else
        {
            // Update height adjustment
            MVCC_INTVALUE height = {0};
            nRet = MV_CC_GetIntValue(device_handle, "Height", &height);
            if (nRet == MV_OK && m_height_adj)
            {
                m_height_adj->set_lower(height.nMin);
                m_height_adj->set_upper(height.nMax);
            }

            snap_and_display(device_handle);
        }
    }
}

std::string MainWindow::convert_to_ip_address_str(uint32_t ip)
{
    std::ostringstream ipStream;
    for (int i = 0; i < 4; ++i)
    {
        if (i > 0)
        {
            ipStream << ".";
        }
        ipStream << ((ip >> (24 - 8 * i)) & 0xFF);
    }
    return ipStream.str();
}

void MainWindow::update_cam_grid()
{
    // Clear camera grid except header
    auto children = m_cam_grid->get_children();
    for (auto* widget : children)
    {
        m_cam_grid->remove(*widget);
    }

    Gtk::Label* status = Gtk::make_managed<Gtk::Label>(Glib::ustring("status"));
    Gtk::Label* model = Gtk::make_managed<Gtk::Label>(Glib::ustring("Model"));
    Gtk::Label* ip = Gtk::make_managed<Gtk::Label>(Glib::ustring("IP Address"));
    Gtk::Label* sn = Gtk::make_managed<Gtk::Label>(Glib::ustring("Serial Number"));
    Gtk::Label* actions = Gtk::make_managed<Gtk::Label>(Glib::ustring("Actions"));

    status->get_style_context()->add_class("header_label");
    model->get_style_context()->add_class("header_label");
    ip->get_style_context()->add_class("header_label");
    sn->get_style_context()->add_class("header_label");
    actions->get_style_context()->add_class("header_label");

    m_cam_grid->attach(*status, 0, 0);
    m_cam_grid->attach(*model, 1, 0);
    m_cam_grid->attach(*ip, 2, 0);
    m_cam_grid->attach(*sn, 3, 0);
    m_cam_grid->attach(*actions, 4, 0);

    // Populate camera grid content
    if (m_cam_list.nDeviceNum > 0)
    {
        int row_index = 1;
        for (unsigned int i = 0; i < m_cam_list.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO *pDeviceInfo = m_cam_list.pDeviceInfo[i];
            if (pDeviceInfo == nullptr)
            {
                continue;
            }

            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
            {
                auto modelName = pDeviceInfo->SpecialInfo.stGigEInfo.chModelName;
                auto friendlyName = pDeviceInfo->SpecialInfo.stGigEInfo.chUserDefinedName;
                auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;

                // Create labels for each row's data
                Gtk::Label* status_data = Gtk::make_managed<Gtk::Label>(Glib::ustring(""));
                Gtk::Label* model_data = Gtk::make_managed<Gtk::Label>(Glib::ustring(reinterpret_cast<const char *>(modelName)));
                Gtk::Label* ip_data = Gtk::make_managed<Gtk::Label>(convert_to_ip_address_str(pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp));
                Gtk::Label* sn_data = Gtk::make_managed<Gtk::Label>(Glib::ustring(reinterpret_cast<const char *>(serialNumber)));

                // Create status indicator
                status_data->set_size_request(16, 16);
                status_data->get_style_context()->add_class("gray-indicator");
                status_data->set_halign(Gtk::ALIGN_CENTER);
                status_data->set_valign(Gtk::ALIGN_CENTER);

                // Attach row data to the grid
                m_cam_grid->attach(*status_data, 0, row_index);
                m_cam_grid->attach(*model_data, 1, row_index);
                m_cam_grid->attach(*ip_data, 2, row_index);
                m_cam_grid->attach(*sn_data, 3, row_index);

                // Create the edit and delete buttons
                auto connect_button = Gtk::make_managed<Gtk::Button>();
                auto disconnect_button = Gtk::make_managed<Gtk::Button>();
                auto view_button = Gtk::make_managed<Gtk::Button>();

                // Load SVG icons from GResource and set them to buttons
                set_button_icon(connect_button, "/com/example/eagle_eye/connect.svg");
                set_button_icon(disconnect_button, "/com/example/eagle_eye/disconnect.svg");
                set_button_icon(view_button, "/com/example/eagle_eye/view.svg");

                // Set buttons' tooltip 
                connect_button->set_tooltip_text("Connect");
                disconnect_button->set_tooltip_text("Disconnect");
                view_button->set_tooltip_text("View Settings");

                // Pack buttons into a horizontal box for the action column
                Gtk::Box* actions_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
                actions_box->pack_start(*connect_button);
                actions_box->pack_start(*disconnect_button);
                actions_box->pack_start(*view_button);

                // Connect button signals
                connect_button->signal_clicked().connect([=] { on_connect_clicked(reinterpret_cast<const char *>(serialNumber)); });
                disconnect_button->signal_clicked().connect([=] { on_disconnect_clicked(reinterpret_cast<const char *>(serialNumber)); });
                view_button->signal_clicked().connect([=] { on_view_clicked(reinterpret_cast<const char *>(serialNumber)); });

                // Attach buttons to grid
                m_cam_grid->attach(*actions_box, 4, row_index);

                row_index++;
            }
        }
    }
    else
    {
        std::cout << "No device found." << std::endl;
        m_logger->log("No device found.");
    }

    m_cam_grid->show_all_children();
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_connect_clicked(const std::string& sn)
{
    if (!connect_camera(sn))
    {
        std::cout << "Failed to connect to the camera: " << sn << std::endl;
        m_logger->log("Failed to connect to the camera: " + sn, Logger::ERROR);
        return;
    }

    // Update status indicator
    Gtk::Label *status_indicator = nullptr;
    size_t children_size = m_cam_grid->get_children().size();
    for (size_t i = 5; i < children_size; i += 5) {  // Each row has 5 columns, skip headers
        size_t row = i / 5;
        Gtk::Label* sn_label = dynamic_cast<Gtk::Label*>(m_cam_grid->get_child_at(3, row));  // Serial number is at column 3
        if (sn_label && sn_label->get_text() == sn) {
            status_indicator = dynamic_cast<Gtk::Label*>(m_cam_grid->get_child_at(0, row));  // Status label is at column 0
            break;
        }
    }
    if (status_indicator)
    {
        status_indicator->get_style_context()->remove_class("gray-indicator");
        status_indicator->get_style_context()->add_class("green-indicator");
    }

    if (!configure_camera(sn))
    {
        std::cout << "Failed to configure the camera: " << sn << std::endl;
        m_logger->log("Failed to configure the camera: " + sn, Logger::ERROR);
        return;
    }

    // Reset image pixel buffer and test frame
    if (m_image_pixbuf_settings)
    {
        m_image_pixbuf_settings.reset();
    }

    // Reset zoom and pan when a new image is loaded
    m_zoom_factor_settings = 1.0;
    m_offset_x_settings = 0.0;
    m_offset_y_settings = 0.0;

    // Start grabbing images
    auto device_handle = create_or_get_device_handle_by_serial_number(sn);
    int nRet = MV_CC_StartGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StartGrabbing: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Capture one frame
    nRet = MV_CC_SetCommandValue(device_handle, "TriggerSoftware");
    if(nRet != MV_OK)
    {
        std::cout << "Error on TriggerSoftware: " << nRet << std::endl;
        m_logger->log("Error on TriggerSoftware: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Grab one frame from camera
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    nRet = MV_CC_GetIntValue(device_handle, "PayloadSize", &stParam);
    if (nRet != MV_OK)
    {
        std::cout << "Get PayloadSize fail! Error code: " << nRet << std::endl;
        return;
    }

    MV_FRAME_OUT_INFO_EX stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    unsigned char *pData = (unsigned char *)malloc(sizeof(unsigned char) * stParam.nCurValue);
    if (pData == nullptr)
    {
        return;
    }

    unsigned int nDataSize = stParam.nCurValue;
    nRet = MV_CC_GetOneFrameTimeout(device_handle, pData, nDataSize, &stImageInfo, 1000);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_GetOneFrameTimeout fail. Error code: " << nRet << std::endl;
        return;
    }

    auto frame_width = stImageInfo.nWidth;
    auto frame_height = stImageInfo.nHeight;

    // Save the frame to file
    save_tmp_image(pData, stImageInfo, device_handle);

    // Load the frame from file    
    try
    {
        m_image_pixbuf_settings = Gdk::Pixbuf::create_from_file("/tmp/eagle_eye/tmp.jpeg");
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "File Error: " << ex.what() << std::endl;
    }
    catch (const Gdk::PixbufError &ex)
    {
        std::cerr << "Pixbuf Error: " << ex.what() << std::endl;
    }

    // Queue the frame for diaplay
    if (m_image_pixbuf_settings)
    {
        m_settings_display_area->set_size_request(frame_width, frame_height);
        m_settings_display_area->queue_draw();
    }

    // Stop grabbing images
    nRet = MV_CC_StopGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
    }
}

void MainWindow::on_disconnect_clicked(const std::string& sn)
{
    if (!disconnect_camera(sn))
    {
        std::cout << "Failed to disconnect from the camera: " << sn << std::endl;
        m_logger->log("Failed to disconnect from the camera: " + sn, Logger::ERROR);
    }
    else
    {
        // Update status indicator
        Gtk::Label *status_indicator = nullptr;
        size_t children_size = m_cam_grid->get_children().size();
        for (size_t i = 5; i < children_size; i += 5) {  // Each row has 5 columns, skip headers
            size_t row = i / 5;
            Gtk::Label* sn_label = dynamic_cast<Gtk::Label*>(m_cam_grid->get_child_at(3, row));  // Serial number is at column 3
            if (sn_label && sn_label->get_text() == sn) {
                status_indicator = dynamic_cast<Gtk::Label*>(m_cam_grid->get_child_at(0, row));  // Status label is at column 0
                break;
            }
        }
        if (status_indicator)
        {
            status_indicator->get_style_context()->remove_class("green-indicator");
            status_indicator->get_style_context()->add_class("gray-indicator");
        }

        clear_camera_settings();
        if (m_image_pixbuf_settings)
        {
            m_image_pixbuf_settings.reset();
            m_settings_display_area->queue_draw();
        }
    }
}

void MainWindow::on_view_clicked(const std::string& sn)
{
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        populate_camera_settings(device_handle);
    }
    else
    {
        std::cout << "Camera with serial number " << sn << " not found." << std::endl;
        show_camera_connect_warning(*this, "The selected camera appears to be disconnected. Please connect it before accessing the settings.");
    }
}

void MainWindow::populate_camera_settings(void *device_handle)
{
    // Serial number
    MVCC_STRINGVALUE sn = {0};
    std::string serial_number = "";
    int nRet = MV_CC_GetStringValue(device_handle, "DeviceSerialNumber", &sn);
    if (nRet == MV_OK && m_sn_lbl)
    {
        serial_number = sn.chCurValue;
        m_sn_lbl->set_text(serial_number);
    }

    // Exposure time
    MVCC_FLOATVALUE exposure_time = {0};
    nRet = MV_CC_GetFloatValue(device_handle, "ExposureTime", &exposure_time);
    if (nRet == MV_OK && m_exposure_time_entry)
    {
        // Convert float to string
        std::ostringstream oss;
        oss << exposure_time.fCurValue;

        // Set the label text
        m_exposure_time_entry->set_text(Glib::ustring(oss.str()));
    }
    else
    {
        std::cout << "Failed to get exposure time. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetFloatValue(ExposureTime): " + std::to_string(nRet), Logger::ERROR);
    }

    // Width
    MVCC_INTVALUE width = {0};
    nRet = MV_CC_GetIntValue(device_handle, "Width", &width);
    if (nRet == MV_OK && m_width_adj && m_width_sb)
    {
        m_width_adj->set_lower(width.nMin);
        m_width_adj->set_upper(width.nMax);
        m_width_adj->set_step_increment(width.nInc);
        m_width_sb->set_value(width.nCurValue);
    }
    else
    {
        std::cout << "Failed to get width. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(Width): " + std::to_string(nRet), Logger::ERROR);
    }

    // Height
    MVCC_INTVALUE height = {0};
    nRet = MV_CC_GetIntValue(device_handle, "Height", &height);
    if (nRet == MV_OK && m_height_adj && m_height_sb)
    {
        m_height_adj->set_lower(height.nMin);
        m_height_adj->set_upper(height.nMax);
        m_height_adj->set_step_increment(height.nInc);
        m_height_sb->set_value(height.nCurValue);
    }
    else
    {
        std::cout << "Failed to get height. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(Height): " + std::to_string(nRet), Logger::ERROR);
    }

    // Offset X
    MVCC_INTVALUE offset_x = {0};
    nRet = MV_CC_GetIntValue(device_handle, "OffsetX", &offset_x);
    if (nRet == MV_OK && m_offset_x_adj && m_offset_x_sb)
    {
        m_offset_x_adj->set_lower(offset_x.nMin);
        m_offset_x_adj->set_upper(offset_x.nMax);
        m_offset_x_adj->set_step_increment(offset_x.nInc);
        m_offset_x_sb->set_value(offset_x.nCurValue);
    }
    else
    {
        std::cout << "Failed to get offsetX. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(OffsetX): " + std::to_string(nRet), Logger::ERROR);
    }

    // Offset Y
    MVCC_INTVALUE offset_y = {0};
    nRet = MV_CC_GetIntValue(device_handle, "OffsetY", &offset_y);
    if (nRet == MV_OK && m_offset_y_adj && m_offset_y_sb)
    {
        m_offset_y_adj->set_lower(offset_y.nMin);
        m_offset_y_adj->set_upper(offset_y.nMax);
        m_offset_y_adj->set_step_increment(offset_y.nInc);
        m_offset_y_sb->set_value(offset_y.nCurValue);
    }
    else
    {
        std::cout << "Failed to get offsetY. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(OffsetY): " + std::to_string(nRet), Logger::ERROR);
    }

    // Load digital IO line number from .ini file
    std::string digital_input_line_number = "";
    std::string digital_output_line_number = "";
    std::ifstream settings_file(AppPaths::Settings_File_Path.string());
    std::string line;
    bool is_current_device = false;

    if (settings_file.is_open())
    {
        while (std::getline(settings_file, line))
        {
            if (line == "[" + serial_number + "]")
            {
                is_current_device = true;
            }
            else if (line.find('[') != std::string::npos)
            {
                is_current_device = false; // New section means we passed the current device's settings
            }

            if (is_current_device)
            {
                std::istringstream line_stream(line);
                std::string key;

                if (std::getline(line_stream, key, '='))
                {
                    std::string value;
                    if (key == "digital_input_line_number" && std::getline(line_stream, value))
                    {
                        digital_input_line_number = value;
                    }
                    else if (key == "digital_output_line_number" && std::getline(line_stream, value))
                    {
                        digital_output_line_number = value;
                    }
                }
            }
        }
        settings_file.close();
    }

    // Digital Input Line Number (fixed to "Line0")
    m_settings_digital_input_line_number_cbox->remove_all();
    m_settings_digital_input_line_number_cbox->append("Line0");
    m_settings_digital_input_line_number_cbox->set_active_text(digital_input_line_number);

    // Digital Input Debounce Time
    if (digital_input_line_number != "")
    {
        nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", digital_input_line_number.c_str());
        if (nRet == MV_OK)
        {
            // Wait a bit or Network error occurs - MV_E_NETER (0x80000206)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            MVCC_INTVALUE debounce_time = {0};
            nRet = MV_CC_GetIntValue(device_handle, "LineDebouncerTime", &debounce_time);
            if (nRet == MV_OK && m_debounce_time_adj && m_settings_digital_input_debouncer_time_sb)
            {
                m_debounce_time_adj->set_lower(debounce_time.nMin);
                m_debounce_time_adj->set_upper(debounce_time.nMax);
                m_debounce_time_adj->set_step_increment(debounce_time.nInc);
                m_settings_digital_input_debouncer_time_sb->set_value(debounce_time.nCurValue);
            }
        }
    }

    // Digital Input Event Trigger
    m_settings_digital_input_event_trigger_cbox->remove_all();

    MVCC_ENUMVALUE event_trigger = {0};
    nRet = MV_CC_GetEnumValue(device_handle, "EventSelector", &event_trigger);
    if (nRet == MV_OK)
    {
        std::string active_text = "";
        for (unsigned int i = 0; i < event_trigger.nSupportedNum; ++i)
        {
            MVCC_ENUMENTRY event_trigger_entry = {0};
            event_trigger_entry.nValue = event_trigger.nSupportValue[i];
            nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "EventSelector", &event_trigger_entry);
            if (nRet == MV_OK)
            {
                std::string trigger = event_trigger_entry.chSymbolic;
                // Only add Digital Input event triggers
                if (trigger == "Line0RisingEdge" || trigger == "Line0FallingEdge")
                {
                    m_settings_digital_input_event_trigger_cbox->append(trigger);
                    if (event_trigger_entry.nValue == event_trigger.nCurValue)
                    {
                        active_text = event_trigger_entry.chSymbolic;
                    }
                }
            }
            else
            {
                std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
            }
        }
        m_settings_digital_input_event_trigger_cbox->set_active_text(active_text);
    }
    else
    {
        std::cerr << "Failed to get event_trigger. Error code: " << nRet << std::endl;
    }

    // Digital Input Event Notification
    m_settings_digital_input_notification_status_cbox->remove_all();

    MVCC_ENUMVALUE notification_status = {0};
    nRet = MV_CC_GetEnumValue(device_handle, "EventNotification", &notification_status);
    if (nRet == MV_OK)
    {
        std::string active_text = "";
        for (unsigned int i = 0; i < notification_status.nSupportedNum; ++i)
        {
            MVCC_ENUMENTRY notification_status_entry = {0};
            notification_status_entry.nValue = notification_status.nSupportValue[i];
            nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "EventNotification", &notification_status_entry);
            if (nRet == MV_OK)
            {
                m_settings_digital_input_notification_status_cbox->append(notification_status_entry.chSymbolic);
                if (notification_status_entry.nValue == notification_status.nCurValue)
                {
                    active_text = notification_status_entry.chSymbolic;
                }
            }
            else
            {
                std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
            }
        }
        m_settings_digital_input_notification_status_cbox->set_active_text(active_text);
    }
    else
    {
        std::cerr << "Failed to get notification_status. Error code: " << nRet << std::endl;
    }

    // Digital Output Line Number (fixed to "Line1" and "Line2")
    m_settings_digital_output_line_number_cbox->remove_all();
    m_settings_digital_output_line_number_cbox->append("Line1");
    m_settings_digital_output_line_number_cbox->append("Line2");
    m_settings_digital_output_line_number_cbox->set_active_text(digital_output_line_number);
    
    // MVCC_ENUMVALUE line_number = {0};
    // nRet = MV_CC_GetEnumValue(device_handle, "LineSelector", &line_number);
    // if (nRet == MV_OK)
    // {
    //     std::string active_text = "";
    //     for (unsigned int i = 0; i < line_number.nSupportedNum; ++i)
    //     {
    //         MVCC_ENUMENTRY line_entry = {0};
    //         line_entry.nValue = line_number.nSupportValue[i];
    //         nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "LineSelector", &line_entry);
    //         if (nRet == MV_OK)
    //         {
    //             m_settings_digital_output_line_number_cbox->append(line_entry.chSymbolic);
    //             if (line_entry.nValue == line_number.nCurValue)
    //             {
    //                 active_text = line_entry.chSymbolic;
    //             }
    //         }
    //         else
    //         {
    //             std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
    //         }
    //     }
    //     m_settings_digital_output_line_number_cbox->set_active_text(active_text);
    // }
    // else
    // {
    //     std::cerr << "Failed to get line_number. Error code: " << nRet << std::endl;
    // }

    if (digital_output_line_number != "")
    {
        nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", digital_output_line_number.c_str());
        if (nRet == MV_OK)
        {
            // Wait a bit or Network error occurs - MV_E_NETER (0x80000206)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Digital Output Line Mode
            m_settings_digital_output_line_mode_cbox->remove_all();

            MVCC_ENUMVALUE line_mode = {0};
            nRet = MV_CC_GetEnumValue(device_handle, "LineMode", &line_mode);
            if (nRet == MV_OK)
            {
                std::string active_text = "";
                for (unsigned int i = 0; i < line_mode.nSupportedNum; ++i)
                {
                    MVCC_ENUMENTRY line_entry = {0};
                    line_entry.nValue = line_mode.nSupportValue[i];
                    nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "LineMode", &line_entry);
                    if (nRet == MV_OK)
                    {
                        m_settings_digital_output_line_mode_cbox->append(line_entry.chSymbolic);
                        if (line_entry.nValue == line_mode.nCurValue)
                        {
                            active_text = line_entry.chSymbolic;
                        }
                    }
                    else
                    {
                        std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
                    }
                }
                m_settings_digital_output_line_mode_cbox->set_active_text(active_text);
            }
            else
            {
                std::cerr << "Failed to get line_mode. Error code: " << nRet << std::endl;
            }

            // Digital Output Line Source
            m_settings_digital_output_line_source_cbox->remove_all();

            MVCC_ENUMVALUE line_source = {0};
            nRet = MV_CC_GetEnumValue(device_handle, "LineSource", &line_source);
            if (nRet == MV_OK)
            {
                std::string active_text = "";
                for (unsigned int i = 0; i < line_source.nSupportedNum; ++i)
                {
                    MVCC_ENUMENTRY line_entry = {0};
                    line_entry.nValue = line_source.nSupportValue[i];
                    nRet = MV_CC_GetEnumEntrySymbolic(device_handle, "LineSource", &line_entry);
                    if (nRet == MV_OK)
                    {
                        m_settings_digital_output_line_source_cbox->append(line_entry.chSymbolic);
                        if (line_entry.nValue == line_source.nCurValue)
                        {
                            active_text = line_entry.chSymbolic;
                        }
                    }
                    else
                    {
                        std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
                    }
                }
                m_settings_digital_output_line_source_cbox->set_active_text(active_text);
            }
            else
            {
                std::cerr << "Failed to get line_source. Error code: " << nRet << std::endl;
            }

            // Strobe Enable
            bool strobe_enable = false;
            nRet = MV_CC_GetBoolValue(device_handle, "StrobeEnable", &strobe_enable);
            if (nRet == MV_OK)
            {
                m_settings_strobe_enable_switch->set_active(strobe_enable);
            }
            else
            {
                std::cerr << "Failed to get strobe_enable. Error code: " << nRet << std::endl;
            }

            // Strobe Duration
            MVCC_INTVALUE strobe_duration = {0};
            nRet = MV_CC_GetIntValue(device_handle, "StrobeLineDuration", &strobe_duration);
            if (nRet == MV_OK && m_strobe_duration_adj && m_settings_strobe_duration_sb)
            {
                m_strobe_duration_adj->set_lower(strobe_duration.nMin);
                m_strobe_duration_adj->set_upper(strobe_duration.nMax);
                m_strobe_duration_adj->set_step_increment(strobe_duration.nInc);
                m_settings_strobe_duration_sb->set_value(strobe_duration.nCurValue);
            }
            else
            {
                std::cout << "Failed to get StrobeLineDuration. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_GetIntValue(StrobeLineDuration): " + std::to_string(nRet), Logger::ERROR);
            }
        }
    }
}

void MainWindow::clear_camera_settings()
{
    if (m_sn_lbl)
    {
        m_sn_lbl->set_text("");
    }
    if (m_exposure_time_entry)
    {
        m_exposure_time_entry->set_text("");
    }
    if (m_width_sb)
    {
        m_width_sb->set_text("");
    }
    if (m_height_sb)
    {
        m_height_sb->set_text("");
    }
    if (m_offset_x_sb)
    {
        m_offset_x_sb->set_text("");
    }
    if (m_offset_y_sb)
    {
        m_offset_y_sb->set_text("");
    }
    if (m_digital_io_type_cbox)
    {
        m_digital_io_type_cbox->set_active_text("");
    }
    if (m_settings_digital_input_line_number_cbox)
    {
        m_settings_digital_input_line_number_cbox->remove_all();
    }
    if (m_settings_digital_input_debouncer_time_sb)
    {
        m_settings_digital_input_debouncer_time_sb->set_text("");
    }
    if (m_settings_digital_input_event_trigger_cbox)
    {
        m_settings_digital_input_event_trigger_cbox->remove_all();
    }
    if (m_settings_digital_input_notification_status_cbox)
    {
        m_settings_digital_input_notification_status_cbox->remove_all();
    }
    if (m_settings_digital_output_line_number_cbox)
    {
        m_settings_digital_output_line_number_cbox->remove_all();
    }
    if (m_settings_digital_output_line_mode_cbox)
    {
        m_settings_digital_output_line_mode_cbox->remove_all();
    }
    if (m_settings_digital_output_line_source_cbox)
    {
        m_settings_digital_output_line_source_cbox->remove_all();
    }
    if (m_settings_strobe_enable_switch)
    {
        m_settings_strobe_enable_switch->set_active(false);
    }
    if (m_settings_strobe_duration_sb)
    {
        m_settings_strobe_duration_sb->set_text("");
    }
}

void MainWindow::set_button_icon(Gtk::Button* button, const Glib::ustring& resource_path)
{
    try {
        // Create a Pixbuf from the SVG file in the resource
        auto pixbuf = Gdk::Pixbuf::create_from_resource(resource_path);

        // Create an Image widget to hold the Pixbuf
        auto image = Gtk::make_managed<Gtk::Image>(pixbuf);

        // Set the image on the button
        button->set_image(*image);
        button->set_relief(Gtk::RELIEF_NONE); // Optional: remove button relief for a cleaner look
    }
    catch (const Glib::Exception& e) {
        std::cerr << "Error loading resource: " << e.what() << std::endl;
    }
}

void MainWindow::on_window_shown()
{
    discover_cameras();
    update_cam_grid();
    if (m_cam_list.nDeviceNum > 0)
    {
        for (unsigned int i = 0; i < m_cam_list.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO *pDeviceInfo = m_cam_list.pDeviceInfo[i];
            if (pDeviceInfo == nullptr)
            {
                continue;
            }

            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
            {
                // Add the camera name to the combo box
                auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
                m_select_detection_camera_cbox->append(std::string((char *)serialNumber));
                m_select_detection_digital_input_cbox->append(std::string((char *)serialNumber));
                m_select_detection_digital_output_cbox->append(std::string((char *)serialNumber));
                m_snap_source_cbox->append(std::string((char *)serialNumber));
                m_toolkit_digital_output_source_cbox->append(std::string((char *)serialNumber));
                m_toolkit_digital_input_sink_cbox->append(std::string((char *)serialNumber));
            }
        }
        m_select_detection_camera_cbox->append("All Cameras");
    }
    else
    {
        std::cout << "No camera found." << std::endl;
        m_logger->log("No camera found.");
    }

    load_detection_results();
    load_detection_settings();
    clear_camera_settings();
    
    // Set up websocket callbacks
    m_ws_client.on_connect([this]() {
        std::cout << "Successfully connected to the WebSocket server!" << std::endl;
        m_is_ws_connected = true;
    });
    m_ws_client.on_disconnect([this]() {
        std::cout << "Disconnected from the WebSocket server." << std::endl;
        m_is_ws_connected = false;
    });
    m_ws_client.on_message_received([this](const std::string &message) {
        std::cout << "Received message: " << message << std::endl;

        // Lock the mutex before modifying shared resource
        {
            std::lock_guard<std::mutex> lock(m_ws_response_mutex);
            m_ws_response = message;
            m_ws_response_ready = true;
        }

        // Notify one waiting thread that the condition is met
        m_ws_response_cv.notify_one();
    });
    
    // Connect to the WebSocket server in a separate thread
    std::thread([this]() {
        std::string uri = "ws://localhost:9001";
        m_ws_client.connect(uri);
    }).detach();  // Detach the thread so it runs independently
}

bool MainWindow::on_window_delete(GdkEventAny* event)
{
    // Disconnect from the WebSocket server
    m_ws_client.disconnect();

    // Returning false allows the window to close
    return false;
}

bool MainWindow::connect_camera(const std::string& sn)
{
    auto device_handle = create_or_get_device_handle_by_serial_number(sn);

    // Check if already connected
    if (MV_CC_IsDeviceConnected(device_handle))
    {
        show_camera_connect_warning(*this, "The selected camera appears to be in use. Please disconnect it from any other application or device before proceeding.");
        return false;
    }

    // Connect to the device
    int nRet = MV_CC_OpenDevice(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_OpenDevice: " + std::to_string(nRet), Logger::ERROR);
    }
    else
    {
        m_logger->log("Connected to camera: " + sn);
        m_connected_device_handles[sn] = device_handle;
    }

    return nRet == MV_OK;
}

bool MainWindow::configure_camera(const std::string sn)
{
    int nRet = 0;
    auto device_handle = create_or_get_device_handle_by_serial_number(sn);

    // Detect network optimal packet size(It only works for the GigE camera)
    int nPacketSize = MV_CC_GetOptimalPacketSize(device_handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValue(device_handle, "GevSCPSPacketSize", nPacketSize);
        if (nRet != MV_OK)
        {
            std::cout << "Set Packet Size fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(GevSCPSPacketSize): " + std::to_string(nRet), Logger::ERROR);
            return false;
        }
    }
    else
    {
        std::cout << "Get Packet Size fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetOptimalPacketSize: " + std::to_string(nRet), Logger::ERROR);
        return false;
    }

    // Enable trigger mode
    nRet = MV_CC_SetEnumValue(device_handle, "TriggerMode", 1);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_SetTriggerMode fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetTriggerMode: " + std::to_string(nRet), Logger::ERROR);
        return false;
    }

    // Set trigger source
    nRet = MV_CC_SetEnumValue(device_handle, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_SetTriggerSource fail! Error code:" << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(TriggerSource): " + std::to_string(nRet), Logger::ERROR);
        return false;
    }

    // Load settings from .ini file
    std::ifstream settings_file(AppPaths::Settings_File_Path.string());
    std::string line;
    bool is_current_device = false;

    if (settings_file.is_open())
    {
        while (std::getline(settings_file, line))
        {
            if (line == "[" + sn + "]")
            {
                is_current_device = true;
            }
            else if (line.find('[') != std::string::npos)
            {
                is_current_device = false; // New section means we passed the current device's settings
            }

            if (is_current_device)
            {
                std::istringstream line_stream(line);
                std::string key;

                if (std::getline(line_stream, key, '='))
                {
                    std::string value;
                    if (key == "exposure_time" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetFloatValue(device_handle, "ExposureTime", std::stof(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set exposure time. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetFloatValue(ExposureTime): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                    }
                    else if (key == "width" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetIntValue(device_handle, "Width", std::stoi(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set width. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetIntValue(Width): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                    }
                    else if (key == "height" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetIntValue(device_handle, "Height", std::stoi(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set height. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetIntValue(Height): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                    }
                    else if (key == "offset_x" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetIntValue(device_handle, "OffsetX", std::stoi(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set offsetX. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetIntValue(OffsetX): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                    }
                    else if (key == "offset_y" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetIntValue(device_handle, "OffsetY", std::stoi(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set offsetY. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetIntValue(OffsetY): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                    }
                    // LineSelector needs to be set before accessing any digital IO settings,
                    // otherwise MV_E_GC_ACCESS (0x80000106) occurs.
                    // So "digital_input_line_number=*" line must be placed before any "digital_input_<setting>=*" line,
                    // Same thing for digital output.
                    else if (key == "digital_input_line_number" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", value.c_str());
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set LineSelector. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetEnumValueByString(LineSelector): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetEnumValueByString(LineSelector) Succeeded" << std::endl;   
                        }
                    }
                    else if (key == "digital_input_debouncer_time" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetIntValue(device_handle, "LineDebouncerTime", std::stoi(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set LineDebouncerTime. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetIntValue(LineDebouncerTime): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetIntValue(LineDebouncerTime) Succeeded" << std::endl;   
                        }
                    }
                    else if (key == "digital_input_event_trigger" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetEnumValueByString(device_handle, "EventSelector", value.c_str());
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set EventSelector. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetEnumValueByString(EventSelector): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetEnumValueByString(EventSelector) Succeeded" << std::endl;   
                        }                        
                    }
                    else if (key == "digital_input_notification_status" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetEnumValueByString(device_handle, "EventNotification", value.c_str());
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set EventNotification. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetEnumValueByString(EventNotification): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetEnumValueByString(EventNotification) Succeeded" << std::endl;   
                        }                         
                    }
                    else if (key == "digital_output_line_number" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", value.c_str());
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set LineSelector. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetEnumValueByString(LineSelector): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetEnumValueByString(LineSelector) Succeeded" << std::endl;   
                        }                       
                    }
                    else if (key == "digital_output_line_mode" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetEnumValueByString(device_handle, "LineMode", value.c_str());
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set LineMode. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetEnumValueByString(LineMode): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetEnumValueByString(LineMode) Succeeded" << std::endl;   
                        } 
                    }
                    else if (key == "digital_output_line_source" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetEnumValueByString(device_handle, "LineSource", value.c_str());
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set LineSource. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetEnumValueByString(LineSource): " + std::to_string(nRet), Logger::ERROR);
                            break;
                        }
                        else
                        {
                            std::cout << "MV_CC_SetEnumValueByString(LineSource) Succeeded" << std::endl;   
                        }                        
                    }
                    else if (key == "digital_output_strobe_enable" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetBoolValue(device_handle, "StrobeEnable", value == "1");
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set StrobeEnable to " << value << " Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetBoolValue(StrobeEnable). Error code: " + std::to_string(nRet), Logger::ERROR);
                        }
                        else
                        {
                            std::cout << "MV_CC_SetBoolValue(StrobeEnable) Succeeded" << std::endl;   
                        }                        
                    }
                    else if (key == "digital_output_strobe_duration" && std::getline(line_stream, value))
                    {
                        nRet = MV_CC_SetIntValue(device_handle, "StrobeLineDuration", std::stoi(value));
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to set StrobeLineDuration to " << std::stoi(value) << " Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetIntValue(StrobeLineDuration). Error code: " + std::to_string(nRet), Logger::ERROR);
                        }
                        else
                        {
                            std::cout << "MV_CC_SetIntValue(StrobeLineDuration) Succeeded" << std::endl;   
                        }
                    }
                }
            }
        }
        settings_file.close();
    }

    return nRet == MV_OK;
}

bool MainWindow::disconnect_camera(const std::string& sn)
{
    if (m_connected_device_handles.find(sn) == m_connected_device_handles.end())
    {
        std::cerr << "Camera cannot be disconnected because it was not connected. Serial Number: " << sn << std::endl;
        return false;
    }

    void *device_handle = m_connected_device_handles[sn];

    // Close the device
    int nRet = MV_CC_CloseDevice(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
        return false;
    }

    // Destory the device handle
    nRet = MV_CC_DestroyHandle(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
        return false;
    }

    m_connected_device_handles.erase(sn);
    return true;
}

void MainWindow::show_camera_connect_warning(Gtk::Window& parent, std::string message)
{
    // Create the message dialog with the specified parent window, message text, and button options
    Gtk::MessageDialog dialog(parent, 
                              message, 
                              false,
                              Gtk::MESSAGE_WARNING,
                              Gtk::BUTTONS_OK,
                              true);

    // Run the dialog and wait for the user to press the OK button
    dialog.run();
}

void MainWindow::discover_cameras()
{
    // enum device
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE, &m_cam_list);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_EnumDevices fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_EnumDevices: " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::on_menu_toggled()
{
    if (m_toolkit_btn->get_active())
    {
        m_content_stack->set_visible_child("page_toolkit");
    }
    else if (m_run_btn->get_active())
    {
        m_content_stack->set_visible_child("page_run");
    }
    else if (m_explore_btn->get_active())
    {
        m_content_stack->set_visible_child("page_explore");
    }
    else if (m_settings_btn->get_active())
    {
        m_content_stack->set_visible_child("page_settings");
    }
}

void MainWindow::on_toolkit_toggled()
{
    if (m_toolkit_anomaly_detection_rbtn->get_active())
    {
        m_toolkit_stack->set_visible_child("page_anomaly_detection");
    }
    else if (m_toolkit_digital_io_rbtn->get_active())
    {
        m_toolkit_stack->set_visible_child("page_digital_io");
    }
}

void MainWindow::on_settings_toggled()
{
    if (m_anomaly_detection_settings_rbtn->get_active())
    {
        m_settings_stack->set_visible_child("page_anomaly_detection_settings");
    }
    else if (m_camera_settings_rbtn->get_active())
    {
        m_settings_stack->set_visible_child("page_camera_settings");
    }
}

void MainWindow::on_start_clicked()
{
    m_is_running = true;

    m_start_btn->set_sensitive(false);
    m_start_btn->set_label("Starting...");

    double capture_interval_ms = 0.0;
    if (m_detection_rate_lbl)
    {
        double capture_rate = std::stod(m_detection_rate_lbl->get_text().raw());
        capture_interval_ms = 1000.0 / capture_rate; // Calculate the capture interval (in milliseconds) based on capture rate (FPS)
    }

    std::vector<std::string> serial_numbers;
    
    if (m_select_detection_camera_cbox)
    {
        auto selected_detection_source = m_select_detection_camera_cbox->get_active_text();
        if (selected_detection_source == "All Cameras")
        {
            auto model = m_select_detection_camera_cbox->get_model();
            if (model)
            {
                for (auto& row : model->children())
                {
                    Glib::ustring sn;
                    row.get_value(0, sn); // 0 is the column index for ComboBoxText items
                    
                    if (sn == "All Cameras")
                    {
                        continue;
                    }

                    serial_numbers.push_back(sn);
                }
            }
        }
        else
        {
            serial_numbers.push_back(selected_detection_source);
        }
    }

    size_t num_connected = 0;
    size_t num_configured = 0;
    size_t total_cams = serial_numbers.size();
    std::vector<std::string> connected_serial_numbers;

    for (const std::string sn : serial_numbers)
    {
        if (!connect_camera(sn))
        {
            std::cerr << "Failed to connect to the camera: " << sn << std::endl;
            m_logger->log("Failed to connect to the camera: " + sn, Logger::ERROR);
            break;
        }
        else
        {
            connected_serial_numbers.push_back(sn);
            num_connected++;
        }
    }

    if (num_connected != total_cams)
    {
        for (const std::string sn : connected_serial_numbers)
        {
            if (!disconnect_camera(sn))
            {
                std::cerr << "Failed to disconnect camera: " << sn << std::endl;
            }
        }
        m_is_running = false;
        m_start_btn->set_sensitive(true);
        m_start_btn->set_label("Start");
        return;
    }

    for (const std::string sn : connected_serial_numbers)
    {
        if (!configure_camera(sn))
        {
            std::cout << "Failed to configure the camera: " << sn << std::endl;
            m_logger->log("Failed to configure the camera: " + sn, Logger::ERROR);
            break;
        }
        else
        {
            num_configured++;
        }
    }

    if (num_configured != total_cams)
    {
        for (const std::string sn : connected_serial_numbers)
        {
            if (!disconnect_camera(sn))
            {
                std::cerr << "Failed to disconnect camera: " << sn << std::endl;
            }
        }
        m_is_running = false;
        m_start_btn->set_sensitive(true);
        m_start_btn->set_label("Start");
        return;
    }

    size_t num_registed = 0;
    for (const std::string sn : connected_serial_numbers)
    {
        void *device_handle = m_connected_device_handles[sn];
        // Register image callback
        auto image_capture_callback = [](unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
        {
            // if (pFrameInfo)
            // {
            //     std::cout << "GetOneFrame, nDevTimeStampHigh: " << pFrameInfo->nDevTimeStampHigh
            //             << ", nDevTimeStampLow: " << pFrameInfo->nDevTimeStampLow
            //             << ", nHostTimeStamp: " << pFrameInfo->nHostTimeStamp
            //             << std::endl;
            // }
            CaptureCallbackData* cb_data = static_cast<CaptureCallbackData*>(pUser);
            MainWindow *ptr = static_cast<MainWindow*>(cb_data->main_window_ptr);
            std::string sn = cb_data->serial_number;
            ptr->m_frame_queue.enqueue(FrameData(pData, pFrameInfo, sn));
        };

        auto cb_data = new CaptureCallbackData(this, sn);
        int nRet = MV_CC_RegisterImageCallBackEx(device_handle, image_capture_callback, cb_data);
        if (nRet != MV_OK)
        {
            std::cerr << "MV_CC_RegisterImageCallBackEx fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_RegisterImageCallBackEx: " + std::to_string(nRet), Logger::ERROR);
            delete cb_data; // Free memory if registration fails
            break;
        }
        num_registed++;
    }

    if (num_registed != total_cams)
    {
        for (const std::string sn : connected_serial_numbers)
        {
            if (!disconnect_camera(sn))
            {
                std::cerr << "Failed to disconnect camera: " << sn << std::endl;
            }
        }
        m_is_running = false;
        m_start_btn->set_sensitive(true);
        m_start_btn->set_label("Start");
        return;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (const std::string sn : connected_serial_numbers)
    {
        void *device_handle = m_connected_device_handles[sn];

        auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cerr << "Begin capture for device: " << sn << " at " << currentTimeInMs << std::endl;
        m_logger->log("Begin capture for device: " + sn);

        start_capture(device_handle, capture_interval_ms);
    }

    start_detection();

    m_start_btn->set_label("Start");
}

void MainWindow::on_stop_clicked()
{
    m_is_running = false;

    // Stop capture thead
    for (const auto& pair : m_connected_device_handles)
    {
        void* device_handle = pair.second;
        m_logger->log("Stop capture for device: " + pair.first);
        stop_capture(device_handle);
    }

    // Stop detection thread
    stop_detection();

    // Disconnect cameras
    std::vector<std::string> serial_numbers;
    for (const auto& pair : m_connected_device_handles) 
    {
        serial_numbers.push_back(pair.first);  // Add each key to the vector
    }

    for (const std::string sn : serial_numbers)
    {
        if (!disconnect_camera(sn))
        {
            std::cout << "Failed to disconnect from the camera: " << sn << std::endl;
            m_logger->log("Failed to disconnect from the camera: " + sn, Logger::ERROR);
        }
    }

    m_start_btn->set_sensitive(!m_is_running);
}

void MainWindow::on_snap_clicked()
{
    // Reset image pixel buffer
    if (m_image_pixbuf_toolkit)
    {
        m_image_pixbuf_toolkit.reset();
    }
    // Reset mask pixel buffer
    if (m_mask_pixbuf_toolkit)
    {
        m_mask_pixbuf_toolkit.reset();
    }
    m_toolkit_display_area->queue_draw();

    auto sn = m_snap_source_cbox->get_active_text();
    if (!connect_camera(sn))
    {
        std::cout << "Failed to connect to the camera: " << sn << std::endl;
        m_logger->log("Failed to connect to the camera: " + sn, Logger::ERROR);
        return;
    }

    if (!configure_camera(sn))
    {
        std::cout << "Failed to configure the camera: " << sn << std::endl;
        m_logger->log("Failed to configure the camera: " + sn, Logger::ERROR);
        return;
    }

    // Start grabbing images
    auto device_handle = create_or_get_device_handle_by_serial_number(sn);
    int nRet = MV_CC_StartGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StartGrabbing: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Capture one frame
    nRet = MV_CC_SetCommandValue(device_handle, "TriggerSoftware");
    if(nRet != MV_OK)
    {
        std::cout << "Error on TriggerSoftware: " << nRet << std::endl;
        m_logger->log("Error on TriggerSoftware: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Grab one frame from camera
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    nRet = MV_CC_GetIntValue(device_handle, "PayloadSize", &stParam);
    if (nRet != MV_OK)
    {
        std::cout << "Get PayloadSize fail! Error code: " << nRet << std::endl;
        return;
    }

    MV_FRAME_OUT_INFO_EX stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    unsigned char *pData = (unsigned char *)malloc(sizeof(unsigned char) * stParam.nCurValue);
    if (pData == nullptr)
    {
        return;
    }

    unsigned int nDataSize = stParam.nCurValue;
    nRet = MV_CC_GetOneFrameTimeout(device_handle, pData, nDataSize, &stImageInfo, 1000);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_GetOneFrameTimeout fail. Error code: " << nRet << std::endl;
        return;
    }

    auto frame_width = stImageInfo.nWidth;
    auto frame_height = stImageInfo.nHeight;

    // Save the frame to file
    save_tmp_image(pData, stImageInfo, device_handle);

    // Load the frame from file    
    try
    {
        m_image_pixbuf_toolkit = Gdk::Pixbuf::create_from_file("/tmp/eagle_eye/tmp.jpeg");
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "File Error: " << ex.what() << std::endl;
    }
    catch (const Gdk::PixbufError &ex)
    {
        std::cerr << "Pixbuf Error: " << ex.what() << std::endl;
    }

    // Reset zoom and pan when a new image is loaded
    m_zoom_factor_toolkit = 1.0;
    m_offset_x_toolkit = 0.0;
    m_offset_y_toolkit = 0.0;
    
    // Start detection
    auto patch_size = 256;
    std::string shm_name = "/ee_shared_memory";
    size_t buffer = 2448 * 2048 * 2;

    // Open shared memory object
    std::cout << "Open shared memory object" << std::endl;
    int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        std::cerr << "Failed to open shared memory object." << std::endl;
        return;
    }

    // Resize shared memory object to the initial data size
    std::cout << "Resize shared memory object" << std::endl;
    if (ftruncate(shm_fd, buffer) == -1)
    {
        std::cerr << "Failed to resize shared memory object." << std::endl;
        ::close(shm_fd);
        return;
    }

    // Map shared memory into address space
    std::cout << "Map shared memory" << std::endl;
    void *shm_ptr = mmap(0, buffer, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        std::cerr << "Failed to map shared memory." << std::endl;
        ::close(shm_fd);
        return;
    }

    std::vector<FrameOffsetInfo> frame_offsets;
    size_t offset = 0;
    int num_frames = frame_height / patch_size;
    auto frame_size = frame_width * patch_size;
    // int num_frames = frame_height / (patch_size * 2);
    // auto frame_size = frame_width * (patch_size * 2);

    for (int i = 0; i < num_frames; ++i)
    {
        // Save the frame metadata
        frame_offsets.push_back({ offset, frame_size, sn });

        // Calculate the memory address to copy this frame
        void* frame_ptr = static_cast<uint8_t*>(shm_ptr) + offset;

        // Copy the frame data into the calculated memory location
        std::memcpy(frame_ptr, pData + offset, frame_size);

        // Update offset for the next frame
        offset += frame_size;
    }

    if (!frame_offsets.empty())
    {
        // Send the command to the ws server
        auto trans_id = generate_transaction_id();

        // Get confidence threshold and pixel threshold from settings file
        double confidence_threshold = 0.5;
        double pixel_threshold = 0.1;

        auto settings = SettingsService::get_settings("[detection]");
        for (const auto &[key, value] : settings)
        {
            if (key == "confidence_threshold")
            {
                confidence_threshold = std::stod(value);
            }
            else if (key == "pixel_threshold")
            {
                pixel_threshold = std::stod(value);
            }
        }

        // Build JSON transaction data
        nlohmann::json json_data;
        json_data["transaction_id"] = trans_id;
        json_data["confidence_threshold"] = confidence_threshold;
        json_data["pixel_threshold"] = pixel_threshold;
        json_data["frame_width"] = frame_width;
        json_data["frame_height"] = patch_size * 2; // adjust frame height for each frame
        for (const auto& info : frame_offsets)
        {
            json_data["frames"].push_back({
                {"offset", info.offset},
                {"frame_size", info.frame_size},
                {"serial_number", info.serial_number}
            });
        }

        std::string frame_info_string = json_data.dump(); // Convert JSON to string
        send_ws_message(frame_info_string);

        // Parse the JSON response
        nlohmann::json response_json = nlohmann::json::parse(m_ws_response);

        // Extract values from the JSON object
        std::string res_trans_id = response_json["transaction_id"];
        std::string status = response_json["status"];
        int total_anomalies = response_json["total_anomalies"];
        
        if (res_trans_id == trans_id && status == "complete" && total_anomalies > 0)
        {
            update_snap_masks(res_trans_id);

            std::string digital_ouput;
            std::string line_number;

            if (m_detection_digital_output_lbl)
            {
                digital_ouput = m_detection_digital_output_lbl->get_text();
            }
            if (m_detection_digital_output_line_number_lbl)
            {
                line_number = m_detection_digital_output_line_number_lbl->get_text();
            }

            if (digital_ouput != "" && 
                line_number != "" && 
                m_connected_device_handles.find(digital_ouput) != m_connected_device_handles.end())
            {
                void *device_handle = m_connected_device_handles[digital_ouput];
                // Select digital output
                int nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", line_number.c_str());
                if (nRet == MV_OK)
                {
                    // Trigger digital output
                    int nRet = MV_CC_SetCommandValue(device_handle, "LineTriggerSoftware");
                    if (nRet != MV_OK)
                    {
                        std::cerr << "Error to send command LineTriggerSoftware. Error code: " << nRet << std::endl;
                        m_logger->log("Error on MV_CC_SetCommandValue(LineTriggerSoftware). Error code: " + std::to_string(nRet), Logger::ERROR);
                    }
                    else
                    {
                        std::cout << "Trigger digital output via software succeeded" << std::endl;
                        m_logger->log("Trigger digital output via software succeeded");
                    }
                }
            }
            else
            {
                std::cerr << "Digital output source not found: " << digital_ouput << std::endl;
                m_logger->log("Digital output source not found: " + digital_ouput, Logger::ERROR);
            }
        }
    }

    if (m_toolkit_display_area)
    {
        m_toolkit_display_area->set_size_request(frame_width, frame_height);
        m_toolkit_display_area->queue_draw();
    }

    // Clean up
    std::cout << "Clean up shared memory object" << std::endl; 
    if (munmap(shm_ptr, buffer) == -1) // Unmap the shared memory
    {
        std::cerr << "Failed to unmap shared memory." << std::endl;
    }
    ::close(shm_fd);

    // Stop grabbing images
    nRet = MV_CC_StopGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StopGrabbing", Logger::ERROR);
    }

    if (!disconnect_camera(sn))
    {
        std::cout << "Failed to disconnect from the camera: " << sn << std::endl;
        m_logger->log("Failed to disconnect from the camera: " + sn, Logger::ERROR);
    }
}

void MainWindow::update_snap_masks(std::string trans_id)
{
    std::filesystem::path trans_json = AppPaths::Detection_Results_Path / trans_id / "transaction_data.json";
    if (!std::filesystem::exists(trans_json))
    {
        std::cerr << "File not exists: transaction_data.json" << std::endl;
        return;
    }

    // Read the content of the JSON file
    std::ifstream json_file(trans_json);
    if (!json_file.is_open())
    {
        std::cerr << "Failed to open the file." << std::endl;
        return;
    }

    // Parse the JSON content
    nlohmann::json json_data;
    json_file >> json_data;

    // Access transaction ID
    std::string transaction_id = json_data["transaction_id"];
    if (transaction_id != trans_id)
    {
        std::cerr << "Transaction ID mismatch." << std::endl;
        return;
    }

    auto total_anomalies = json_data["total_anomalies"];
    if (total_anomalies <= 0)
    {
        std::cout << "No anomaly found." << std::endl;
        return;
    }

    int patch_size = json_data["patch_size"].get<int>();
    int frame_width = json_data["frame_width"].get<int>();
    int num_frames = json_data["num_frames"].get<int>();
    int total_height = json_data["frame_height"].get<int>() * num_frames;

    // Create a transparent mask pixbuf of the same size as the image
    m_mask_pixbuf_toolkit = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    // m_mask_pixbuf_toolkit->fill(0xffffffbe); // For testing
    m_mask_pixbuf_toolkit->fill(0x00000000); // Initialize the mask to be fully transparent black

    int predictions_per_row = frame_width / patch_size;
    if (frame_width % patch_size != 0)
    {
        predictions_per_row++; // Allow for an additional prediction if there's remaining space
    }

    // Access predictions array
    for (const auto &prediction : json_data["predictions"])
    {
        int prediction_id = prediction["prediction_id"].get<int>();
        std::string filename = prediction["file_name"].get<std::string>();
        
        // Load the prediction image
        auto prediction_pixbuf = Gdk::Pixbuf::create_from_file(filename);
        if (!prediction_pixbuf)
        {
            std::cerr << "Failed to load prediction image" << std::endl;
            continue;
        }

        // Calculate row and column based on the index
        int row = prediction_id / predictions_per_row;
        int col = prediction_id % predictions_per_row;

        // Calculate position_x
        int position_x = col * patch_size; // Standard position in the row

        // Adjust position_x if this is the last column and it exceeds frame width
        if (col == predictions_per_row - 1 && position_x + patch_size > frame_width)
        {
            position_x = frame_width - patch_size;
        }

        // Calculate position_y
        int position_y = row * patch_size; // Each row is separated by the height of the patch

        // Copy the prediction image into m_mask_pixbuf_toolkit at the specified position
        prediction_pixbuf->Gdk::Pixbuf::copy_area(
            0,
            0,
            prediction_pixbuf->get_width(),
            prediction_pixbuf->get_height(),
            m_mask_pixbuf_toolkit,
            position_x,
            position_y
        );
    }

    // Update mask pixel buf
    update_mask_color(m_mask_pixbuf_toolkit);
    update_mask_alpha(m_mask_pixbuf_toolkit, m_mask_alpha * 255);
}

void MainWindow::update_mask_color(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf)
{
    if (!mask_pixbuf)
        return;

    const int AmberRed = 255;
    const int AmberGreen = 191;
    const int AmberBlue = 0;

    // Get pixbuf properties
    int mask_width = mask_pixbuf->get_width();
    int mask_height = mask_pixbuf->get_height();
    int mask_rowstride = mask_pixbuf->get_rowstride();
    int mask_n_channels = mask_pixbuf->get_n_channels();

    // Get pointer to the pixel data
    guchar *pixels = mask_pixbuf->get_pixels();

    // Iterate through the pixels and modify the alpha channel
    for (int y = 0; y < mask_height; ++y)
    {
        for (int x = 0; x < mask_width; ++x)
        {
            guchar *pixel = pixels + y * mask_rowstride + x * mask_n_channels;

            if (pixel[0] > 0 && pixel[1] > 0 && pixel[2] > 0)
            {
                pixel[0] = AmberRed;
                pixel[1] = AmberGreen;
                pixel[2] = AmberBlue;
            }
        }
    }
}

void MainWindow::update_mask_alpha(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf, gint32 alpha)
{
    if (!mask_pixbuf)
        return;

    // Get pixbuf properties
    int width = mask_pixbuf->get_width();
    int height = mask_pixbuf->get_height();
    int rowstride = mask_pixbuf->get_rowstride();
    int n_channels = mask_pixbuf->get_n_channels();

    // Get pointer to the pixel data
    guchar *pixels = mask_pixbuf->get_pixels();

    // Iterate through the pixels and modify the alpha channel
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            guchar *pixel = pixels + y * rowstride + x * n_channels;

            // Set alpha to 0 for black pixel (RGB = 0,0,0)
            if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
            {
                pixel[3] = 0;
            }
            else if (pixel[3] > 0)
            {
                pixel[3] = alpha;
            }
        }
    }
}

void *MainWindow::create_or_get_device_handle_by_serial_number(std::string sn)
{
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        return m_connected_device_handles[sn];
    } 
    else
    {
        for (unsigned int i = 0; i < m_cam_list.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO *pDeviceInfo = m_cam_list.pDeviceInfo[i];
            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
            {
                auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
                std::string serialNumberStr(reinterpret_cast<const char *>(serialNumber));
                if (serialNumberStr == sn)
                {
                    void *device_handle;
                    int nRet = MV_CC_CreateHandle(&device_handle, pDeviceInfo);
                    if (nRet != MV_OK)
                    {
                        std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
                        m_logger->log("Error on MV_CC_CreateHandle: " + std::to_string(nRet), Logger::ERROR);
                        return nullptr;
                    }
                    return device_handle;
                }
            }
        }
    }
    

    return nullptr;
}

void MainWindow::start_capture(void *device_handle, double capture_interval_ms)
{
    // Start grab images
    int nRet = MV_CC_StartGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StartGrabbing: " + std::to_string(nRet), Logger::ERROR);
    }
    else
    {
        std::string camera_handle = std::to_string(reinterpret_cast<uintptr_t>(device_handle));
        m_logger->log("Started MV_CC_StartGrabbing for device: " + camera_handle);

        // Start the frame acquisition thread
        auto capturing_thread = std::thread([this, device_handle, capture_interval_ms]()
        {
            uint64_t lastCaptureTimestamp = 0;

            while (m_is_running)
            {
                auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                double elapsed = currentTimeInMs - lastCaptureTimestamp;

                if (elapsed >= capture_interval_ms)
                {
                    lastCaptureTimestamp = currentTimeInMs;

                    int nRet = MV_CC_SetCommandValue(device_handle, "TriggerSoftware");
                    if (nRet != MV_OK)
                    {
                        std::cerr << "Failed to capture frames via TriggerSoftware. Error code: " << nRet << std::endl;
                        m_logger->log("Error on MV_CC_SetCommandValue(TriggerSoftware): " + std::to_string(nRet), Logger::ERROR);
                    }
                    else
                    {
                        // std::cout << "Capturing frames via TriggerSoftware." << std::endl;
                    }
                }

                // Prevent CPU overuse
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        m_capturing_threads[camera_handle] = std::move(capturing_thread);
    }
}

void MainWindow::stop_capture(void *device_handle)
{
    std::string camera_handle = std::to_string(reinterpret_cast<uintptr_t>(device_handle));
    auto& capturing_thread = m_capturing_threads[camera_handle];

    if (capturing_thread.joinable()) {
        capturing_thread.join();  // Wait for previous thread to finish
    }

    int nRet = MV_CC_StopGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StopGrabbing", Logger::ERROR);
    }

    m_capturing_threads.erase(camera_handle);
}

void MainWindow::start_detection()
{
    // Ensure there's no existing processing thread running
    if (m_processing_thread.joinable()) 
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }

    // Start the frame processing thread
    m_processing_thread = std::thread([this]()
    {
        FrameData frame_data(nullptr, nullptr, ""); // Initialize FrameData with null pointers

        std::string shm_name = "/ee_shared_memory";
        size_t buffer = 2448 * 2048 * 3;

        // Open shared memory object
        std::cout << "Open shared memory object" << std::endl;
        int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1)
        {
            std::cerr << "Failed to open shared memory object." << std::endl;
            return;
        }

        // Resize shared memory object to the initial data size
        std::cout << "Resize shared memory object" << std::endl;
        if (ftruncate(shm_fd, buffer) == -1)
        {
            std::cerr << "Failed to resize shared memory object." << std::endl;
            ::close(shm_fd);
            return;
        }

        // Map shared memory into address space
        std::cout << "Map shared memory" << std::endl;
        void *shm_ptr = mmap(0, buffer, PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED)
        {
            std::cerr << "Failed to map shared memory." << std::endl;
            ::close(shm_fd);
            return;
        }

        std::vector<FrameOffsetInfo> frame_offsets;
        
        while (m_is_running)
        {
            // std::cout << "Entering processing loop" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
            
            size_t offset = 0;
            frame_offsets.clear();
            int frames_dequeued = 0;
            int frame_width = 0;
            int frame_height = 0;

            // Copy each frame into its respective memory offset
            while (!m_frame_queue.isEmpty() && frames_dequeued < 2)
            {
                if (m_frame_queue.dequeue(frame_data))
                {
                    auto frame_size = frame_data.pMetadata->nFrameLen;
                    frame_width = frame_data.pMetadata->nWidth;
                    frame_height = frame_data.pMetadata->nHeight;
                    auto serial_number = frame_data.serial_number;

                    // Save the frame metadata
                    frame_offsets.push_back({ offset, frame_size, serial_number });

                    // Calculate the memory address to copy this frame
                    void* frame_ptr = static_cast<uint8_t*>(shm_ptr) + offset;

                    // Copy the frame data into the calculated memory location
                    std::memcpy(frame_ptr, frame_data.pData, frame_size);

                    // Update offset for the next frame
                    offset += frame_size;

                    // Increment the counter
                    frames_dequeued++;
                }
            }

            if (frame_offsets.empty())
            {
                continue;
            }

            // Generate a transaction ID
            auto trans_id = generate_transaction_id();

            // Get confidence threshold and pixel threshold from settings file
            double confidence_threshold = 0.5;
            double pixel_threshold = 0.1;

            auto settings = SettingsService::get_settings("[detection]");
            for (const auto &[key, value] : settings)
            {
                if (key == "confidence_threshold")
                {
                    confidence_threshold = std::stod(value);
                }
                else if (key == "pixel_threshold")
                {
                    pixel_threshold = std::stod(value);
                }
            }

            // Send the command to the ws server
            nlohmann::json json_data;
            json_data["transaction_id"] = trans_id;
            json_data["confidence_threshold"] = confidence_threshold;
            json_data["pixel_threshold"] = pixel_threshold;
            json_data["frame_width"] = frame_width;
            json_data["frame_height"] = frame_height;
            for (const auto& info : frame_offsets)
            {
                json_data["frames"].push_back({
                    {"offset", info.offset},
                    {"frame_size", info.frame_size},
                    {"serial_number", info.serial_number}
                });
            }
            std::string frame_info_string = json_data.dump(); // Convert JSON to string
            send_ws_message(frame_info_string);
            
            // Parse the JSON response
            nlohmann::json response_json = nlohmann::json::parse(m_ws_response);

            // Extract values from the JSON object
            std::string res_trans_id = response_json["transaction_id"];
            std::string status = response_json["status"];
            int total_anomalies = response_json["total_anomalies"];
            auto serial_numbers = response_json["serial_numbers"];

            if (res_trans_id == trans_id && status == "complete" && total_anomalies > 0)
            {
                std::string digital_ouput;
                std::string line_number;

                if (m_detection_digital_output_lbl)
                {
                    digital_ouput = m_detection_digital_output_lbl->get_text();
                }
                if (m_detection_digital_output_line_number_lbl)
                {
                    line_number = m_detection_digital_output_line_number_lbl->get_text();
                }

                if (digital_ouput != "" && 
                    line_number != "" && 
                    m_connected_device_handles.find(digital_ouput) != m_connected_device_handles.end())
                {
                    void *device_handle = m_connected_device_handles[digital_ouput];
                    // Select digital output
                    int nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", line_number.c_str());
                    if (nRet == MV_OK)
                    {
                        // Trigger digital output
                        int nRet = MV_CC_SetCommandValue(device_handle, "LineTriggerSoftware");
                        if (nRet != MV_OK)
                        {
                            std::cerr << "Error to send command LineTriggerSoftware. Error code: " << nRet << std::endl;
                            m_logger->log("Error on MV_CC_SetCommandValue(LineTriggerSoftware). Error code: " + std::to_string(nRet), Logger::ERROR);
                        }
                        else
                        {
                            std::cout << "Trigger digital output via software succeeded" << std::endl;
                            m_logger->log("Trigger digital output via software succeeded");
                        }
                    }
                }
                else
                {
                    std::cerr << "Digital output source not found: " << digital_ouput << std::endl;
                    m_logger->log("Digital output source not found: " + digital_ouput, Logger::ERROR);
                }
            }

            // std::cout << "Exiting processing loop" << std::endl;
        }

        // Clean up
        std::cout << "Clean up shared memory object" << std::endl; 
        if (munmap(shm_ptr, buffer) == -1) // Unmap the shared memory
        {
            std::cerr << "Failed to unmap shared memory." << std::endl;
        }
        ::close(shm_fd);
    });
}

void MainWindow::stop_detection()
{
    if (m_processing_thread.joinable()) 
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }

    RetentionManager retention_manager;
    retention_manager.enforce_daily_limit();
}

void MainWindow::send_ws_message(std::string message)
{
    if (m_is_ws_connected)
    {
        std::cout << "sending a ws message: " << message << std::endl;
        m_logger->log("sending a ws message: " + message);
        m_ws_client.send_message(message);

        // wait until receive the response
        std::unique_lock<std::mutex> lock(m_ws_response_mutex);
        m_ws_response_cv.wait(lock, [this]{ return m_ws_response_ready; });
        
        // print the response
        std::cout << "receiving a ws response: " << m_ws_response << std::endl;
        m_logger->log("receiving a ws response: " + m_ws_response);

        // Reset the condition for future use if needed
        m_ws_response_ready = false;
    }
    else
    {
        std::cerr << "Failed to send a message, ws is not connected" << std::endl;
        m_logger->log("Failed to send a message, ws is not connected", Logger::ERROR);
    }
}

void MainWindow::save_tmp_image(unsigned char *pData, MV_FRAME_OUT_INFO_EX frameInfo, void *deviceHandle)
{
    std::string temp_dir = "/tmp/eagle_eye";
    if (!std::filesystem::is_directory(temp_dir))
    {
        if (!std::filesystem::create_directory(temp_dir))
        {
            std::cerr << "Failed to create temporary directory: " << temp_dir << std::endl;
            return;
        }
    }

    MV_SAVE_IMG_TO_FILE_PARAM stSaveFileParam;
    memset(&stSaveFileParam, 0, sizeof(MV_SAVE_IMG_TO_FILE_PARAM));

    stSaveFileParam.enImageType = MV_Image_Jpeg;
    stSaveFileParam.nQuality = 90;
    stSaveFileParam.enPixelType = frameInfo.enPixelType;
    stSaveFileParam.nWidth = frameInfo.nWidth;
    stSaveFileParam.nHeight = frameInfo.nHeight;
    stSaveFileParam.nDataLen = frameInfo.nFrameLen;
    stSaveFileParam.pData = pData;
    sprintf(stSaveFileParam.pImagePath, "/tmp/eagle_eye/tmp.jpeg");
    
    int nRet = MV_CC_SaveImageToFile(deviceHandle, &stSaveFileParam);
    if (nRet != MV_OK)
    {
        std::cerr << "Failed to save image to file. Error code: " << nRet << std::endl;
    }
}

std::string MainWindow::generate_transaction_id()
{
    // Get the current time point with high resolution
    auto now = std::chrono::high_resolution_clock::now();

    // Get the duration since epoch in milliseconds
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // Create a transaction ID using milliseconds since epoch
    std::stringstream ss;
    ss << "TX" << milliseconds; // Prefix with "TX"

    return ss.str();
}

bool MainWindow::on_toolkit_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x_toolkit, m_offset_y_toolkit);   // Apply panning offset
    cr->scale(m_zoom_factor_toolkit, m_zoom_factor_toolkit); // Apply zoom

    // Draw the image
    if (m_image_pixbuf_toolkit)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_toolkit, 0, 0);
        cr->paint();
    }

    if (m_mask_pixbuf_toolkit)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_mask_pixbuf_toolkit, 0, 0);
        cr->paint();
    }

    return true;
}

bool MainWindow::on_key_press_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrl_pressed = true;
    }
    return Gtk::Window::on_key_press_event(key_event);
}

bool MainWindow::on_key_release_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrl_pressed = false;
    }
    return Gtk::Window::on_key_release_event(key_event);
}

bool MainWindow::on_toolkit_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging_toolkit = true;
        m_drag_start_x_toolkit = button_event->x;
        m_drag_start_y_toolkit = button_event->y;
    }
    return true;
}

bool MainWindow::on_toolkit_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging_toolkit = false;
    }
    return true;
}

bool MainWindow::on_toolkit_display_area_scroll_event(GdkEventScroll *scroll_event)
{
    if (m_ctrl_pressed)
    {
        // Adjust alpha when Ctrl is pressed
        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_mask_alpha = std::min(m_mask_alpha + 0.1, 1.0); // Max alpha is 1.0
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_mask_alpha = std::max(m_mask_alpha - 0.1, 0.1); // Min alpha is 0.1
        }

        update_mask_alpha(m_mask_pixbuf_toolkit, m_mask_alpha * 255);
    }
    else
    {
        const double zoom_step = 0.1;

        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_zoom_factor_toolkit += zoom_step;
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_zoom_factor_toolkit = std::max(zoom_step, m_zoom_factor_toolkit - zoom_step);
        }
    }

    // Trigger a redraw of the drawing area
    m_toolkit_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_toolkit_display_area_motion_notify_event(GdkEventMotion *motion_event)
{
    if (m_is_dragging_toolkit)
    {
        // Calculate the distance moved
        double deltaX = motion_event->x - m_drag_start_x_toolkit;
        double deltaY = motion_event->y - m_drag_start_y_toolkit;

        // Update the panning offset
        m_offset_x_toolkit += deltaX;
        m_offset_y_toolkit += deltaY;

        // Update the start position for the next motion event
        m_drag_start_x_toolkit = motion_event->x;
        m_drag_start_y_toolkit = motion_event->y;
    }

    // Trigger a redraw of the drawing area
    m_toolkit_display_area->queue_draw();

    return true;
}

bool MainWindow::on_settings_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x_settings, m_offset_y_settings);   // Apply panning offset
    cr->scale(m_zoom_factor_settings, m_zoom_factor_settings); // Apply zoom

    // Draw the image
    if (m_image_pixbuf_settings)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_settings, 0, 0);
        cr->paint();
    }

    return true;
}

bool MainWindow::on_settings_display_area_scroll_event(GdkEventScroll *scroll_event)
{
    const double zoom_step = 0.1;

    if (scroll_event->direction == GDK_SCROLL_UP)
    {
        m_zoom_factor_settings += zoom_step;
    }
    else if (scroll_event->direction == GDK_SCROLL_DOWN)
    {
        m_zoom_factor_settings = std::max(zoom_step, m_zoom_factor_settings - zoom_step);
    }

    // Trigger a redraw of the drawing area
    m_settings_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_settings_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging_settings = true;
        m_drag_start_x_settings = button_event->x;
        m_drag_start_y_settings = button_event->y;
    }
    return true;
}

bool MainWindow::on_settings_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging_settings = false;
    }
    return true;
}

bool MainWindow::on_settings_display_area_motion_notify_event(GdkEventMotion *motion_event)
{
    if (m_is_dragging_settings)
    {
        // Calculate the distance moved
        double deltaX = motion_event->x - m_drag_start_x_settings;
        double deltaY = motion_event->y - m_drag_start_y_settings;

        // Update the panning offset
        m_offset_x_settings += deltaX;
        m_offset_y_settings += deltaY;

        // Update the start position for the next motion event
        m_drag_start_x_settings = motion_event->x;
        m_drag_start_y_settings = motion_event->y;
    }

    // Trigger a redraw of the drawing area
    m_settings_display_area->queue_draw();

    return true;
}

bool MainWindow::on_detection_display_area_scroll_event(GdkEventScroll *scroll_event)
{
    if (m_ctrl_pressed)
    {
        // Adjust alpha when Ctrl is pressed
        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_mask_alpha = std::min(m_mask_alpha + 0.1, 1.0); // Max alpha is 1.0
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_mask_alpha = std::max(m_mask_alpha - 0.1, 0.1); // Min alpha is 0.1
        }

        update_mask_alpha(m_mask_pixbuf_detection_result, m_mask_alpha * 255);
    }
    else
    {
        const double zoom_step = 0.1;

        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_zoom_factor_detection += zoom_step;
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_zoom_factor_detection = std::max(zoom_step, m_zoom_factor_detection - zoom_step);
        }
    }

    // Trigger a redraw of the drawing area
    m_detection_results_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_detection_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging_detection = true;
        m_drag_start_x_detection = button_event->x;
        m_drag_start_y_detection = button_event->y;
    }
    return true;
}

bool MainWindow::on_detection_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging_detection = false;
    }
    return true;
}

bool MainWindow::on_detection_display_area_motion_notify_event(GdkEventMotion *motion_event)
{
    if (m_is_dragging_detection)
    {
        // Calculate the distance moved
        double deltaX = motion_event->x - m_drag_start_x_detection;
        double deltaY = motion_event->y - m_drag_start_y_detection;

        // Update the panning offset
        m_offset_x_detection += deltaX;
        m_offset_y_detection += deltaY;

        // Update the start position for the next motion event
        m_drag_start_x_detection = motion_event->x;
        m_drag_start_y_detection = motion_event->y;
    }

    // Trigger a redraw of the drawing area
    m_detection_results_display_area->queue_draw();

    return true;
}