#include "main_window.h"
#include "app_paths.h"
#include "settings_service.h"
#include "retention_manager.h"
#include "constants.h"
#include "time_utils.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_frame_queue(2),
      m_logger(logger),
      m_main_images_dispatcher(),
      m_main_masks_dispatcher()
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
    set_window_title(APP_NAME);

    m_builder->get_widget("startup_rbtn", m_startup_btn);
    if (m_startup_btn)
    {
        m_startup_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("runtime_rbtn", m_runtime_btn);
    if (m_runtime_btn)
    {
        m_runtime_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
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

    m_builder->get_widget("runtime_stack", m_runtime_stack);

    m_builder->get_widget("new_project_btn", m_new_project_btn);
    if (m_new_project_btn)
    {
        m_new_project_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_new_project_clicked));
    }

    m_builder->get_widget("open_project_btn", m_open_project_btn);
    if (m_open_project_btn)
    {
        m_open_project_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_open_project_clicked));
    }

    m_builder->get_widget("quick_start_btn", m_quick_start_btn);
    if (m_quick_start_btn)
    {
        m_quick_start_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_quick_start_clicked));
    }

    m_builder->get_widget("recent_projects_listbox", m_recent_projects_listbox);
    if (m_recent_projects_listbox)
    {
        m_recent_projects_listbox->signal_row_selected().connect(sigc::mem_fun(*this, &MainWindow::on_recent_project_selected));
    }

    m_builder->get_widget("runtime_no_project_lbl", m_runtime_no_project_lbl);

    m_builder->get_widget("runtime_nav_button_box", m_runtime_nav_button_box);

    m_builder->get_widget("runtime_control_panel_rbtn", m_runtime_control_panel_rbtn);
    if (m_runtime_control_panel_rbtn)
    {
        m_runtime_control_panel_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_runtime_tab_clicked));
    }

    m_builder->get_widget("runtime_monitoring_rbtn", m_runtime_monitoring_rbtn);
    if (m_runtime_monitoring_rbtn)
    {
        m_runtime_monitoring_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_runtime_tab_clicked));
    }

    m_builder->get_widget("runtime_report_rbtn", m_runtime_report_rbtn);
    if (m_runtime_report_rbtn)
    {
        m_runtime_report_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_runtime_tab_clicked));
    }

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

    m_builder->get_widget("rt_monitoring_detection_start_time_lbl", m_rt_monitoring_detection_start_time_lbl);

    m_builder->get_widget("rt_monitoring_num_anomalies_lbl", m_rt_monitoring_num_anomalies_lbl);

    m_builder->get_widget("rt_monitoring_drawing_area", m_rt_monitoring_drawing_area);
    if (m_rt_monitoring_drawing_area)
    {
        m_rt_monitoring_drawing_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_rt_monitoring_display_area_draw));

        // Connect mouse scroll event
        m_rt_monitoring_drawing_area->add_events(Gdk::SCROLL_MASK);
        m_rt_monitoring_drawing_area->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_rt_monitoring_display_area_scroll_event));

        // Connect mouse press and motion events
        m_rt_monitoring_drawing_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_rt_monitoring_drawing_area->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_rt_monitoring_display_area_btn_press_event));
        m_rt_monitoring_drawing_area->signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_rt_monitoring_display_area_btn_release_event));
        m_rt_monitoring_drawing_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_rt_monitoring_display_area_motion_notify_event));
    }

    m_builder->get_widget("report_refresh_btn", m_report_refresh_btn);
    if (m_report_refresh_btn)
    {
        m_report_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_report_refresh_clicked));
    }

    m_builder->get_widget("report_transactions_listbox", m_report_transactions_listbox);
    if (m_report_transactions_listbox)
    {
        m_report_transactions_listbox->signal_row_selected().connect(sigc::mem_fun(*this, &MainWindow::on_transaction_selected));
    }

    m_builder->get_widget("report_display_area", m_report_image_display_area);
    if (m_report_image_display_area)
    {
        m_report_image_display_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_report_display_area_draw));

        // Connect mouse scroll event
        m_report_image_display_area->add_events(Gdk::SCROLL_MASK);
        m_report_image_display_area->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_report_display_area_scroll_event));

        // Connect mouse press and motion events
        m_report_image_display_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_report_image_display_area->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_report_display_area_btn_press_event));
        m_report_image_display_area->signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_report_display_area_btn_release_event));
        m_report_image_display_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_report_display_area_motion_notify_event));
    }

    m_builder->get_widget("report_masking_switch", m_report_masking_switch);
    if (m_report_masking_switch)
    {
        // Get the PropertyProxy for the active property of the switch
        Glib::PropertyProxy<bool> active_property = m_report_masking_switch->property_active();

        // Connect to the signal_changed() of the PropertyProxy
        active_property.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_report_enable_masking_changed));
    }

    m_builder->get_widget("report_patches_box", m_report_patches_box);

    m_builder->get_widget("report_position_display_area", m_report_position_display_area);
    if (m_report_position_display_area)
    {
        m_report_position_display_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_report_position_draw));
    }

    m_builder->get_widget("moving_speed_entry", m_moving_speed_entry);

    m_builder->get_widget("save_report_btn", m_save_report_btn);
    if (m_save_report_btn)
    {
        m_save_report_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_save_report_clicked));
    }

    m_builder->get_widget("snap_source_cbox", m_snap_source_cbox);

    m_builder->get_widget("snap_btn", m_snap_btn);
    if (m_snap_btn)
    {
        m_snap_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_snap_clicked));
    }

    m_builder->get_widget("toolkit_image_picker_fcb", m_toolkit_image_picker_fcb);
    if (m_toolkit_image_picker_fcb)
    {
        m_toolkit_image_picker_fcb->signal_selection_changed().connect([this]()
        {
            // Reset zoom and pan when a new image is loaded
            m_zoom_factor_toolkit = 1.0;
            m_offset_x_toolkit = 0.0;
            m_offset_y_toolkit = 0.0;

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

            // Load the frame from file    
            auto image_under_test = m_toolkit_image_picker_fcb->get_filename();
            try
            {
                m_image_pixbuf_toolkit = Gdk::Pixbuf::create_from_file(image_under_test);
            }
            catch (const Glib::FileError &ex)
            {
                std::cerr << "File Error: " << ex.what() << std::endl;
                return;
            }
            catch (const Gdk::PixbufError &ex)
            {
                std::cerr << "Pixbuf Error: " << ex.what() << std::endl;
                return;
            }

            if (!m_image_pixbuf_toolkit)
            {
                std::cerr << "Failed to load the image!" << std::endl;
                return;
            }

            auto frame_width = m_image_pixbuf_toolkit->get_width();
            auto frame_height = m_image_pixbuf_toolkit->get_height();
            
            if (m_toolkit_display_area)
            {
                m_toolkit_display_area->set_size_request(frame_width, frame_height);
                m_toolkit_display_area->queue_draw();
            }
        });
    }

    m_builder->get_widget("toolkit_detection_test_btn", m_toolkit_detection_test_btn);
    if (m_toolkit_detection_test_btn)
    {
        m_toolkit_detection_test_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_toolkit_test_clicked));
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

    m_builder->get_widget("cam_settings_grid", m_cam_settings_grid);

    m_builder->get_widget("sn_lbl", m_sn_lbl);
    if (m_sn_lbl)
    {
        // Get the PropertyProxy for the active property of the switch
        auto label_property = m_sn_lbl->property_label();

        // Connect to the signal_changed() of the PropertyProxy
        label_property.signal_changed().connect([this]() {
            m_cam_settings_grid->set_sensitive(!m_sn_lbl->get_text().empty());
        });
    }

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
        // Get the PropertyProxy for the active property of the switch
        Glib::PropertyProxy<bool> active_property = m_settings_strobe_enable_switch->property_active();

        // Connect to the signal_changed() of the PropertyProxy
        active_property.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_strobe_enable_state_set));
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

    m_builder->get_widget("detection_patches_box", m_detection_patches_box);

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

    m_builder->get_widget("detection_results_masking_switch", m_detection_results_masking_switch);
    if (m_detection_results_masking_switch)
    {
        // Get the PropertyProxy for the active property of the switch
        Glib::PropertyProxy<bool> active_property = m_detection_results_masking_switch->property_active();

        // Connect to the signal_changed() of the PropertyProxy
        active_property.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_enable_masking_changed));
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
    if (m_settings_digital_input_line_number_cbox)
    {
        m_settings_digital_input_line_number_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_digital_input_line_number_changed));
    }

    m_builder->get_widget("settings_digital_input_debouncer_time_sb", m_settings_digital_input_debouncer_time_sb);

    m_builder->get_widget("settings_digital_input_event_trigger_cbox", m_settings_digital_input_event_trigger_cbox);

    m_builder->get_widget("settings_digital_input_event_status_cbox", m_settings_digital_input_notification_status_cbox);

    m_builder->get_widget("digital_input_event_status_lbl", m_digital_input_event_status_lbl);

    m_builder->get_widget("digital_input_event_tv", m_digital_input_event_tv);
}

void MainWindow::set_window_title(const std::string &title)
{
    Gtk::Window *root;
    m_builder->get_widget("main_window", root);
    root->set_title(title);
}

void MainWindow::on_runtime_tab_clicked()
{
    if (m_runtime_control_panel_rbtn->get_active())
    {
        m_runtime_stack->set_visible_child("page_rt_control_panel");
    }
    else if (m_runtime_monitoring_rbtn->get_active())
    {
        m_runtime_stack->set_visible_child("page_rt_monitoring");
    }
    else if (m_runtime_report_rbtn->get_active())
    {
        m_runtime_stack->set_visible_child("page_rt_report");
    }
}

void MainWindow::on_report_refresh_clicked()
{
    // Load transactions list
    m_sorted_detection_results_in_report = FileUtils::get_folders_by_time(AppPaths::Project_Detection_Results_Path(m_curr_project_name), std::chrono::system_clock::time_point::min(), std::chrono::system_clock::now());

    // Clear the results before loading
    for (auto *child : m_report_transactions_listbox->get_children())
    {
        m_report_transactions_listbox->remove(*child);
    }

    // Populating the detection results list box with rows
    for (const auto &result_folder : m_sorted_detection_results_in_report)
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
        listbox_row->set_margin_top(5);
        listbox_row->set_margin_start(5);
        listbox_row->set_margin_end(5);
        // Add the Gtk::ListBoxRow to the list box
        m_report_transactions_listbox->append(*listbox_row);
        // Show all the newly added widgets
        listbox_row->show_all();
    }

    // Select the first row
    auto most_recent_result = m_report_transactions_listbox->get_row_at_index(0);
    if (most_recent_result)
    {
        m_report_transactions_listbox->select_row(*most_recent_result);
    }

    // Load position track
    if (m_report_position_display_area)
    {
        m_report_position_display_area->queue_draw();
    }
}

void MainWindow::on_report_enable_masking_changed()
{
    m_show_mask_in_report = m_report_masking_switch->get_active();
    Gtk::ListBoxRow* selected_row = m_report_transactions_listbox->get_selected_row();
    if (selected_row)
    {
        on_transaction_selected(selected_row);
    }
}

bool MainWindow::on_report_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x_report, m_offset_y_report);   // Apply panning offset
    cr->scale(m_zoom_factor_report, m_zoom_factor_report); // Apply zoom

    // Draw the images
    if (m_image_pixbuf_report)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_report, 0, 0);
        cr->paint();
    }

    // Draw the masks
    if (m_show_mask_in_report && m_mask_pixbuf_report)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_mask_pixbuf_report, 0, 0);
        cr->paint();
    }

    return true;
}

bool MainWindow::on_report_display_area_scroll_event(GdkEventScroll *scroll_event)
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

        update_mask_alpha(m_mask_pixbuf_report, m_mask_alpha * 255);
    }
    else
    {
        const double zoom_step = 0.1;

        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_zoom_factor_report += zoom_step;
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_zoom_factor_report = std::max(zoom_step, m_zoom_factor_report - zoom_step);
        }
    }

    // Trigger a redraw of the drawing area
    m_report_image_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_report_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging_report = true;
        m_drag_start_x_report = button_event->x;
        m_drag_start_y_report = button_event->y;
    }

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_report_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging_report = false;
    }
    
    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_report_display_area_motion_notify_event(GdkEventMotion *motion_event)
{
    if (m_is_dragging_report)
    {
        // Calculate the distance moved
        double deltaX = motion_event->x - m_drag_start_x_report;
        double deltaY = motion_event->y - m_drag_start_y_report;

        // Update the panning offset
        m_offset_x_report += deltaX;
        m_offset_y_report += deltaY;

        // Update the start position for the next motion event
        m_drag_start_x_report = motion_event->x;
        m_drag_start_y_report = motion_event->y;
    }

    // Trigger a redraw of the drawing area
    m_report_image_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, double>> MainWindow::track_position(double speed)
{
    std::map<std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point>, 
             std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, double>>> session_time_points_with_positions;

    for (const auto &transaction_folder : m_sorted_detection_results_in_report)
    {
        auto trans_file_path = std::filesystem::path(transaction_folder) / "transaction_data.json";
        auto json_data = FileUtils::get_json(trans_file_path.string());
        if (!json_data.has_value())
        {
            continue;
        }

        const auto &json = json_data.value();
        if (!json.contains("transaction_id"))
        {
            continue;
        }

        auto transaction_id = json["transaction_id"];
        auto transaction_tp = FileUtils::get_transaction_time(transaction_folder);
        
        if (!transaction_tp.has_value())
        {
            continue;
        }

        auto transaction_tp_value = transaction_tp.value();
        // Find the corresponding session
        for (const auto &session : m_session_times)
        {
            const auto &start = session.first;
            const auto &end = session.second;

            // Check if 'transaction_tp' falls into the session
            if (transaction_tp_value >= start && transaction_tp_value < end)
            {
                // Assign the time point to the session
                session_time_points_with_positions[session].emplace_back(transaction_id, transaction_tp_value, 0.0);
                break; // Stop searching once the session is found
            }
        }
    }

    double position = 0.0; // Initialize the position globally (shared across all sessions)

    for (auto &entry : session_time_points_with_positions)
    {
        auto &session = entry.first; // Key: {start_time, end_time}
        auto &transactions = entry.second; // Value: list of {transaction_id, time_point, position}

        // Access session start and end times
        const auto &start_time = session.first;

        // Iterate through transactions in the session
        for (auto &transaction : transactions)
        {
            auto &transaction_id = std::get<0>(transaction);
            const auto &time_point = std::get<1>(transaction);
            auto &current_position = std::get<2>(transaction);

            // Calculate precise elapsed time in milliseconds
            auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - start_time).count();

            // Update position
            current_position = position + speed * elapsed_time / 1000.0;
        }

        // Update global position
        auto last_transaction = transactions.back(); 
        position = std::get<2>(last_transaction);
    }

    std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, double>> transactions_with_positions;
    
    // Flatten all transactions into the result vector
    for (const auto &[session, transactions] : session_time_points_with_positions)
    {
        for (const auto &[transaction_id, time_point, current_position] : transactions)
        {
            transactions_with_positions.emplace_back(transaction_id, time_point, current_position);
        }
    }

    return transactions_with_positions;
}

bool MainWindow::on_report_position_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Get the DrawingArea dimensions
    int width = m_report_position_display_area->get_allocated_width();
    int height = m_report_position_display_area->get_allocated_height();

    // Draw the position track (horizontal line)
    cr->set_line_width(height);
    cr->set_source_rgb(0, 0, 0); // Black
    cr->move_to(0, height / 2);
    cr->line_to(width, height / 2);
    cr->stroke();

    auto transactions_with_positions = track_position(1.0); // unit speed

    if (transactions_with_positions.empty())
    {
        std::cout << "No transactions available." << std::endl;
        return true;
    }
    
    const auto &last_transaction = transactions_with_positions.back();
    const auto &position = std::get<2>(last_transaction);
    auto total_distance = position;

    // Draw transactions on position track
    for (const auto &transaction : transactions_with_positions)
    {
        const auto &transaction_id = std::get<0>(transaction);
        const auto &time_point = std::get<1>(transaction);
        const auto &position = std::get<2>(transaction);

        auto x = position * width / total_distance;

        std::cout << "position: " << position << ", width: " << width << ", total distance: " << total_distance << std::endl;

        // Draw the vertical line
        cr->set_line_width(2.0);
        cr->set_source_rgb(1.0, 0.5, 0.0); // Amber
        cr->move_to(x, 0);
        cr->line_to(x, height);
        cr->stroke();

        // Highlight selected result
        if (m_selected_transaction_id == transaction_id)
        {
            const double triangle_size = 10.0; // Size of the triangle
            cr->set_source_rgb(1.0, 0.5, 0.0); // Amber
            cr->move_to(x, 15);                // Top point of the triangle
            cr->line_to(x - triangle_size, 0); // Bottom-left point
            cr->line_to(x + triangle_size, 0); // Bottom-right point
            cr->close_path();
            cr->fill();
        }
    }

    return true;
}

void MainWindow::on_save_report_clicked()
{
    // Create a FileChooserDialog in Save mode
    Gtk::FileChooserDialog dialog("Save Report", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);

    // Set the initial folder
    dialog.set_current_folder(AppPaths::Project_Path(m_curr_project_name));

    // Add a filter to show only CSV files
    auto csv_filter = Gtk::FileFilter::create();
    csv_filter->set_name("CSV files");
    csv_filter->add_pattern("*.csv");
    dialog.add_filter(csv_filter);

    // Add buttons for user actions
    dialog.add_button("_Cancel", Gtk::ResponseType::RESPONSE_REJECT);
    dialog.add_button("_Save", Gtk::ResponseType::RESPONSE_ACCEPT);

    // Set default filename
    dialog.set_current_name("detection_report.csv");

    // Enable overwrite confirmation
    dialog.set_do_overwrite_confirmation(true);

    // Show the dialog and wait for user response
    int result = dialog.run();

    switch (result)
    {
        case Gtk::ResponseType::RESPONSE_ACCEPT:
        {
            // Get the selected file path
            std::string file_path = dialog.get_filename();
            std::cout << "File selected to save: " << file_path << std::endl;

            // Save the file
            create_csv_file(file_path);
            break;
        }
        case Gtk::ResponseType::RESPONSE_REJECT:
            std::cout << "Save operation canceled." << std::endl;
            break;

        default:
            std::cout << "Unexpected response." << std::endl;
            break;
    }
}

void MainWindow::create_csv_file(const std::string &file_name)
{
    // Open the file for writing
    std::ofstream csv_file(file_name);

    if (!csv_file.is_open())
    {
        std::cerr << "Failed to open file: " << file_name << std::endl;
        return;
    }

    // Write the moving speed in the first row
    double moving_speed;
    try
    {
        moving_speed = std::stod(m_moving_speed_entry->get_text());
        csv_file << "Moving Speed (mm/s):," << moving_speed << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return;
    }

    // Write the headers
    csv_file << "Transaction ID,Date Time (yyyy-mm-dd hh:mm:ss.000), Position (mm)\n";

    if (m_session_times.empty())
    {
        std::cerr << "No active session to create report." << std::endl;
        return;
    }

    auto start_time = m_session_times.front().first;
    auto end_time = m_session_times.back().second;
    auto transactions_with_positions = track_position(moving_speed);

    // Write the first row
    csv_file << "N/A" << ','
             << '=' << '"' << TimeUtils::get_formatted_time(start_time) << '"' << ','
             << 0 << '\n';

    // Write transactions
    for (const auto &transaction : transactions_with_positions)
    {
        const auto &transaction_id = std::get<0>(transaction);
        const auto &time_point = std::get<1>(transaction);
        const auto &position = std::get<2>(transaction);

        csv_file << transaction_id << ','
             << '=' << '"' << TimeUtils::get_formatted_time(time_point) << '"' << ','
             << position << '\n';
    }

    // Write the last row
    csv_file << "N/A" << ','
             << '=' << '"' << TimeUtils::get_formatted_time(start_time) << '"' << ','
             << "N/A" << '\n';

    // Close the file
    csv_file.close();
    std::cout << "CSV file created successfully: " << file_name << std::endl;
}

void MainWindow::on_transaction_selected(Gtk::ListBoxRow* row)
{
    if (row)
    {
        auto row_box = dynamic_cast<Gtk::Box*>(row->get_child());
        if (row_box)
        {
            auto trans_label = dynamic_cast<Gtk::Label *>(row_box->get_children()[0]);
            if (trans_label)
            {
                m_selected_transaction_id = trans_label->get_text();
                std::string selected_transaction_path = row_box->get_tooltip_text();
                load_detection_result_in_report(selected_transaction_path);

                // Redraw position track and indicator
                if (m_report_position_display_area)
                {
                    m_report_position_display_area->queue_draw();
                }
            }
        }
    }
    else
    {
        std::cout << "No row selected!" << std::endl;
    }
}

bool MainWindow::on_rt_monitoring_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x_rt_monitoring, m_offset_y_rt_monitoring);   // Apply panning offset
    cr->scale(m_zoom_factor_rt_monitoring, m_zoom_factor_rt_monitoring); // Apply zoom

    // Draw the image
    if (m_image_pixbuf_rt_monitoring)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_rt_monitoring, 0, 0);
        cr->paint();
    }

    // Draw the mask
    if (m_mask_pixbuf_rt_monitoring)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_mask_pixbuf_rt_monitoring, 0, 0);
        cr->paint();
    }

    return true;
}

bool MainWindow::on_rt_monitoring_display_area_scroll_event(GdkEventScroll *scroll_event)
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

        update_mask_alpha(m_mask_pixbuf_rt_monitoring, m_mask_alpha * 255);
    }
    else
    {
        const double zoom_step = 0.1;

        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_zoom_factor_rt_monitoring += zoom_step;
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_zoom_factor_rt_monitoring = std::max(zoom_step, m_zoom_factor_rt_monitoring - zoom_step);
        }
    }

    // Trigger a redraw of the drawing area
    m_rt_monitoring_drawing_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_rt_monitoring_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging_rt_monitoring = true;
        m_drag_start_x_rt_monitoring = button_event->x;
        m_drag_start_y_rt_monitoring = button_event->y;
    }

    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_rt_monitoring_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging_rt_monitoring = false;
    }
    
    // Return true to indicate that the event has been handled
    return true;
}

bool MainWindow::on_rt_monitoring_display_area_motion_notify_event(GdkEventMotion *motion_event)
{
    if (m_is_dragging_rt_monitoring)
    {
        // Calculate the distance moved
        double deltaX = motion_event->x - m_drag_start_x_rt_monitoring;
        double deltaY = motion_event->y - m_drag_start_y_rt_monitoring;

        // Update the panning offset
        m_offset_x_rt_monitoring += deltaX;
        m_offset_y_rt_monitoring += deltaY;

        // Update the start position for the next motion event
        m_drag_start_x_rt_monitoring = motion_event->x;
        m_drag_start_y_rt_monitoring = motion_event->y;
    }

    // Trigger a redraw of the drawing area
    m_rt_monitoring_drawing_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

void MainWindow::on_detection_digital_input_selection_changed()
{
    std::string digital_input = m_select_detection_digital_input_cbox->get_active_text();
    std::string digital_input_line_number;

    if (digital_input != "")
    {
        auto cam_settings = SettingsService::get_settings(digital_input);
        if (!cam_settings.empty() && cam_settings.contains("digital_input_line_number"))
        {
            digital_input_line_number = cam_settings["digital_input_line_number"];
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
        auto cam_settings = SettingsService::get_settings(digital_output);
        if (!cam_settings.empty() && cam_settings.contains("digital_output_line_number"))
        {
            digital_output_line_number = cam_settings["digital_output_line_number"];
        }
    }

    if (m_settings_detection_digital_output_line_number_lbl)
    {
        m_settings_detection_digital_output_line_number_lbl->set_text(digital_output_line_number);
    }       
}

void MainWindow::load_recent_projects()
{
    // Clear the recent projects before loading
    for (auto *child : m_recent_projects_listbox->get_children())
    {
        m_recent_projects_listbox->remove(*child);
    }

    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> projects_with_times;
    try
    {
        if (std::filesystem::exists(AppPaths::Projects_Path) && std::filesystem::is_directory(AppPaths::Projects_Path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(AppPaths::Projects_Path))
            {
                // Check if the entry is a directory
                if (std::filesystem::is_directory(entry.status()))
                {
                    auto project_dir = entry.path();
                    for (const auto& file_entry : std::filesystem::directory_iterator(project_dir))
                    {
                        auto file_path = file_entry.path();
                        if (file_path.extension() == ".dscanproj")
                        {
                            std::string project_name;
                            auto last_modified_time = std::chrono::system_clock::time_point::min();
                            auto project_json_optional = FileUtils::get_json(file_path.string());
                            if (project_json_optional.has_value())
                            {
                                auto &project_json = project_json_optional.value();
                                if (project_json.contains("project_name"))
                                {
                                    project_name = project_json["project_name"];
                                }
                                if (project_json.contains("last_modified_time"))
                                {
                                    auto last_modified_time_str = project_json["last_modified_time"];
                                    last_modified_time = TimeUtils::parse_time(last_modified_time_str);
                                }
                                projects_with_times.emplace_back(project_name, last_modified_time);
                            }
                        }
                    }
                    
                }
            }

            // Sort projects by last_modified_time in descending order
            std::sort(projects_with_times.begin(), projects_with_times.end(),
                      [](const auto& a, const auto& b) {
                          return a.second > b.second; // Descending order
                      });

            // Get the top five last modified projects
            if (projects_with_times.size() > 5) 
            {
                projects_with_times.resize(5);
            }

            // Populating the recent projects list box with rows
            for (const auto &entry : projects_with_times)
            {
                const auto project_name = entry.first;
                auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
                auto project_label = Gtk::make_managed<Gtk::Label>(project_name);
                row_box->set_tooltip_text(AppPaths::Project_Path(project_name).string());
                row_box->pack_start(*project_label, Gtk::PACK_SHRINK);
                // Create a Gtk::ListBoxRow to wrap the box
                auto listbox_row = Gtk::make_managed<Gtk::ListBoxRow>();
                listbox_row->add(*row_box);
                // Set margin around the row
                listbox_row->set_margin_top(5);      // Space above the row
                listbox_row->set_margin_start(5);   // Space to the left of the row
                listbox_row->set_margin_end(5);     // Space to the right of the row
                // Add a custom CSS class to the row
                auto row_context = listbox_row->get_style_context();
                row_context->add_class("clickable-row");
                // Add the Gtk::ListBoxRow to the list box
                m_recent_projects_listbox->append(*listbox_row);

                // Show all the newly added widgets
                listbox_row->show_all();
            }

            // Add more... button
            auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
            auto more_label = Gtk::make_managed<Gtk::Label>("more...");
            row_box->pack_start(*more_label, Gtk::PACK_SHRINK);
            // Create a Gtk::ListBoxRow to wrap the box
            auto listbox_row = Gtk::make_managed<Gtk::ListBoxRow>();
            listbox_row->add(*row_box);
            // Set margin around the row
            listbox_row->set_margin_top(5);      // Space above the row
            listbox_row->set_margin_start(5);   // Space to the left of the row
            listbox_row->set_margin_end(5);     // Space to the right of the row
            // Add a custom CSS class to the row
            auto row_context = listbox_row->get_style_context();
            row_context->add_class("clickable-row");
            // Add the Gtk::ListBoxRow to the list box
            m_recent_projects_listbox->append(*listbox_row);

            // Show all the newly added widgets
            listbox_row->show_all();
        }
        else
        {
            std::cerr << "Projects_Path does not exist or is not a directory." << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

void MainWindow::on_recent_detection_results_selector_changed()
{
    load_detection_results();
}

void MainWindow::on_detection_results_refresh_clicked()
{
    load_detection_results();
}

void MainWindow::load_detection_results()
{
    // Clear the resutls before loading
    for (auto *child : m_detection_results_listbox->get_children())
    {
        m_detection_results_listbox->remove(*child);
    }

    if (m_recent_detection_results_selector_cbox)
    {
        auto selected_time_range = m_recent_detection_results_selector_cbox->get_active_text();
        std::string start_time;
        std::string end_time = TimeUtils::get_current_time();

        if (selected_time_range == "Past 15 Minutes")
        {
            start_time = TimeUtils::get_time_minutes_ago(15);
        }
        else if (selected_time_range == "Past 1 Hour")
        {
            start_time = TimeUtils::get_time_hours_ago(1);
        }
        else if (selected_time_range == "Past 4 Hours")
        {
            start_time = TimeUtils::get_time_hours_ago(4);
        }
        else if (selected_time_range == "Past 8 Hours")
        {
            start_time = TimeUtils::get_time_hours_ago(8);
        }
        else if (selected_time_range == "Past 24 Hours")
        {
            start_time = TimeUtils::get_time_hours_ago(24);
        }

        auto start_tp = TimeUtils::parse_time(start_time);
        auto end_tp = TimeUtils::parse_time(end_time);

        // Load transactions list
        std::vector<std::filesystem::path> recent_results_folders;

        try
        {
            if (std::filesystem::exists(AppPaths::Projects_Path) && std::filesystem::is_directory(AppPaths::Projects_Path))
            {
                for (const auto& entry : std::filesystem::directory_iterator(AppPaths::Projects_Path))
                {
                    // Check if the entry is a directory
                    if (std::filesystem::is_directory(entry.status()))
                    {
                        auto project_name = entry.path().filename().string();
                        auto results = FileUtils::get_folders_by_time(AppPaths::Project_Detection_Results_Path(project_name), start_tp, end_tp);
                        
                        // Add the results to recent_results_folders
                        recent_results_folders.insert(recent_results_folders.end(), results.begin(), results.end());
                    }
                }
            }
            else
            {
                std::cerr << "Projects_Path does not exist or is not a directory." << std::endl;
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
        
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
            // std::cout << "Selected row: " << result_folder << std::endl;
            load_detection_result_in_explorer(result_folder);
        }
    }
    else
    {
        std::cout << "No row selected!" << std::endl;
    }
}

// nlohmann::json parse_json(const std::filesystem::path& json_path) {
//     if (!std::filesystem::exists(json_path)) {
//         throw std::runtime_error("File not found: " + json_path.string());
//     }

//     std::ifstream json_file(json_path);
//     if (!json_file.is_open()) {
//         throw std::runtime_error("Failed to open the file.");
//     }

//     nlohmann::json json_data;
//     json_file >> json_data;
//     return json_data;
// }

// Glib::RefPtr<Gdk::Pixbuf> create_combined_pixbuf(
//     const nlohmann::json& frames,
//     int frame_width, 
//     int frame_height, 
//     int total_height) 
// {
//     auto pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
//     pixbuf->fill(0x00000000); // Initialize with black

//     int current_y = 0;
//     for (const auto& frame : frames) {
//         auto path = frame["file_name"].get<std::string>();
//         auto frame_pixbuf = Gdk::Pixbuf::create_from_file(path, frame_width, frame_height);
//         frame_pixbuf->copy_area(0, 0, frame_width, frame_height, pixbuf, 0, current_y);
//         current_y += frame_height;
//     }
//     return pixbuf;
// }

// Glib::RefPtr<Gdk::Pixbuf> create_mask_pixbuf(int width, int height) {
//     auto pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
//     pixbuf->fill(0x00000000); // Transparent black
//     return pixbuf;
// }

// void add_patch_to_box(Gtk::Box* patches_box, 
//                       int prediction_id, 
//                       const Glib::RefPtr<Gdk::Pixbuf>& pixbuf, 
//                       const std::function<void()>& on_focus_click, 
//                       const std::function<void()>& on_delete_click) 
// {
//     auto item_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL);
//     item_box->set_spacing(5);

//     auto label = Gtk::make_managed<Gtk::Label>("Patch ID: " + std::to_string(prediction_id));
//     label->set_halign(Gtk::ALIGN_START);
//     item_box->pack_start(*label, Gtk::PACK_SHRINK);

//     auto thumbnail_pixbuf = pixbuf->scale_simple(80, 80, Gdk::INTERP_BILINEAR);
//     auto thumbnail = Gtk::make_managed<Gtk::Image>(thumbnail_pixbuf);
//     thumbnail->set_halign(Gtk::ALIGN_START);
//     item_box->pack_start(*thumbnail, Gtk::PACK_SHRINK);

//     auto action_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
//     action_box->set_spacing(10);

//     auto view_button = Gtk::make_managed<Gtk::Button>();
//     view_button->signal_clicked().connect(on_focus_click);
//     set_button_icon(view_button, "/com/example/eagle_eye/focus.svg");
//     action_box->pack_start(*view_button, Gtk::PACK_SHRINK);

//     auto delete_button = Gtk::make_managed<Gtk::Button>();
//     delete_button->signal_clicked().connect(on_delete_click);
//     set_button_icon(delete_button, "/com/example/eagle_eye/delete.svg");
//     action_box->pack_start(*delete_button, Gtk::PACK_SHRINK);

//     item_box->pack_start(*action_box, Gtk::PACK_SHRINK);
//     patches_box->pack_start(*item_box, Gtk::PACK_SHRINK);
// }

// void MainWindow::load_detection_result2(std::string& detection_result_folder) {
//     try {
//         auto json_path = std::filesystem::path(detection_result_folder) / "transaction_data.json";
//         auto json_data = parse_json(json_path);

//         int patch_size = json_data["patch_size"];
//         int frame_width = json_data["frame_width"];
//         int frame_height = json_data["frame_height"];
//         int num_frames = json_data["num_frames"];
//         int total_height = frame_height * num_frames;

//         m_image_pixbuf_explorer = create_combined_pixbuf(
//             json_data["frames"], frame_width, frame_height, total_height);

//         m_mask_pixbuf_explorer = create_mask_pixbuf(frame_width, total_height);

//         m_detection_patches_box->foreach([](Gtk::Widget& child) {
//             m_detection_patches_box->remove(child);
//         });

//         for (const auto& prediction : json_data["predictions"]) {
//             int prediction_id = prediction["prediction_id"];
//             std::string filename = prediction["file_name"];
//             auto prediction_pixbuf = Gdk::Pixbuf::create_from_file(filename);

//             add_patch_to_box(
//                 m_detection_patches_box,
//                 prediction_id,
//                 prediction_pixbuf,
//                 [this, prediction_id]() { /* Focus callback */ },
//                 [this, prediction_id]() { /* Delete callback */ });
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
// }

void MainWindow::load_detection_result_in_report(std::string &detection_result_folder)
{
    std::filesystem::path trans_json_path = std::filesystem::path(detection_result_folder) / "transaction_data.json";
    if (!std::filesystem::exists(trans_json_path))
    {
        std::cerr << "File not exists: transaction_data.json" << std::endl;
        return;
    }

    // Read the content of the JSON file
    std::ifstream json_file(trans_json_path);
    if (!json_file.is_open())
    {
        std::cerr << "Failed to open the file." << std::endl;
        return;
    }

    // Parse the JSON content
    nlohmann::json json_data;
    int patch_size;
    int frame_width;
    int frame_height;
    int num_frames;
    int total_height;

    try
    {
        json_file >> json_data;
        json_file.close();

        patch_size = json_data["patch_size"].get<int>();
        frame_width = json_data["frame_width"].get<int>();
        frame_height = json_data["frame_height"].get<int>();
        num_frames = json_data["num_frames"].get<int>();
        total_height = frame_height * num_frames;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return;
    }

    // Create the combined pixbuf for detection images.
    // Gdk::Pixbuf does not directly support a single-channel format, 
    // so still create an RGB pixbuf and replicate the grayscale values across the three color channels.
    m_image_pixbuf_report.reset();
    m_image_pixbuf_report = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    m_image_pixbuf_report->fill(0x00000000); // Fill with black

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
            m_image_pixbuf_report, 
            0, 
            current_y);

        pixbuf_image.reset();

        // Update the y-offset for the next image
        current_y += frame_height;
    }

    if (load_images_error)
    {
        // TODO, show a popup
        return;
    }

    // Create a transparent mask pixbuf of the same size as the image
    m_mask_pixbuf_report.reset();
    m_mask_pixbuf_report = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    // m_mask_pixbuf_explorer->fill(0xffffffbe); // For testing
    m_mask_pixbuf_report->fill(0x00000000); // Initialize the mask to be fully transparent black

    // Clear patches box before adding
    for (auto *child : m_report_patches_box->get_children())
    {
        m_report_patches_box->remove(*child);
    }

    // Load and position each prediction
    int predictions_per_row = frame_width / patch_size;
    if (frame_width % patch_size != 0)
    {
        predictions_per_row++; // Allow for an additional prediction if there's remaining space
    }

    // Preserve the original frame pixbuf
    m_image_pixbuf_report_original.reset();
    m_image_pixbuf_report_original = m_image_pixbuf_report->copy();
    // Extract transaction ID
    auto transaction_id = json_data["transaction_id"].get<std::string>();
    
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
        int patch_width = prediction_pixbuf->get_width();
        int patch_height = prediction_pixbuf->get_height();
        prediction_pixbuf->Gdk::Pixbuf::copy_area(
            0,
            0,
            patch_width,
            patch_height,
            m_mask_pixbuf_report,
            position_x,
            position_y
        );

        // Create a vertical box for each item
        auto item_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL);
        item_box->set_spacing(5); // Spacing between child elements

        // Create and add the title label
        auto label = Gtk::make_managed<Gtk::Label>("Patch ID: " + std::to_string(prediction_id));
        label->set_halign(Gtk::ALIGN_START); // Align text to the left
        item_box->pack_start(*label, Gtk::PACK_SHRINK);

        // Create and add the thumbnail
        const int thumbnail_width = 80;
        const int thumbnail_height = 80;
        auto thumbnail_pixbuf = prediction_pixbuf->scale_simple(
            thumbnail_width, 
            thumbnail_height, 
            Gdk::INTERP_BILINEAR);
        auto thumbnail = Gtk::make_managed<Gtk::Image>(thumbnail_pixbuf);
        thumbnail->set_halign(Gtk::ALIGN_START);
        item_box->pack_start(*thumbnail, Gtk::PACK_SHRINK);

        prediction_pixbuf.reset();

        // Create a horizontal box for buttons
        auto action_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
        action_box->set_spacing(10); // Spacing between buttons

        // Add "Focus" button
        auto view_button = Gtk::make_managed<Gtk::Button>();
        view_button->set_margin_top(5);
        view_button->signal_clicked().connect([this, label, position_x, position_y, patch_width, patch_height]() {
            try 
            {
                // Remove the "highlighted" class from the currently highlighted label
                if (m_current_selected_patch_in_report_lbl)
                {
                    m_current_selected_patch_in_report_lbl->get_style_context()->remove_class("highlighted");
                }

                // Highlight the new label
                auto style_context = label->get_style_context();
                style_context->add_class("highlighted");

                // Update the currently highlighted label
                m_current_selected_patch_in_report_lbl = label;

                // Create a Cairo surface based on the existing pixbuf
                auto surface = Cairo::ImageSurface::create(
                    Cairo::FORMAT_ARGB32,
                    m_image_pixbuf_report->get_width(),
                    m_image_pixbuf_report->get_height());
                auto cr = Cairo::Context::create(surface);

                // Clear existing drawings by re-rendering the original pixbuf
                Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_report_original, 0, 0); // Use the original pixbuf
                cr->paint();

                // Set the stroke color (e.g., red)
                cr->set_source_rgba(1.0, 0.0, 0.0, 1.0); // RGBA: red, fully opaque

                // Set line width for the rectangle edges
                cr->set_line_width(2.0);

                // Draw the edges of the rectangle
                cr->move_to(position_x, position_y); // Top-left corner
                cr->line_to(position_x + patch_width, position_y); // Top edge
                cr->line_to(position_x + patch_width, position_y + patch_height); // Right edge
                cr->line_to(position_x, position_y + patch_height); // Bottom edge
                cr->close_path(); // Close the rectangle (connect back to top-left)

                // Stroke the rectangle edges
                cr->stroke();

                // Update the pixbuf with the modified surface
                m_image_pixbuf_report = Gdk::Pixbuf::create(
                    surface, 0, 0,
                    surface->get_width(),
                    surface->get_height());

                // Refresh the UI with the updated pixbuf
                m_report_image_display_area->queue_draw();
            }
            catch (const Glib::Error& ex)
            {
                std::cerr << "Error drawing rectangle: " << ex.what() << std::endl;
            }
        });
        set_button_icon(view_button, "/com/example/eagle_eye/focus.svg");
        action_box->pack_start(*view_button, Gtk::PACK_SHRINK);

        // Add "Delete" button
        auto delete_button = Gtk::make_managed<Gtk::Button>();
        delete_button->set_margin_top(5);
        
        // Load button icon
        auto remark = prediction.value("remark", "TP");
        if (remark == "FP") // Marked as False Positive, the available action is to revert it back to True Positive.
        {
            set_button_icon(delete_button, "/com/example/eagle_eye/confirm.svg");
            update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 128);
        }
        else // 'remark' does not exist or was True Positive, the available action is to mark it as False Positive.
        {
            set_button_icon(delete_button, "/com/example/eagle_eye/delete.svg");
            update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 255);
        }

        delete_button->signal_clicked().connect([this, delete_button, trans_json_path, transaction_id, prediction_id, thumbnail, thumbnail_pixbuf, position_x, position_y, patch_width, patch_height]()
        {
            try
            {
                std::string filename = transaction_id + "_patch_" + std::to_string(prediction_id) + ".png";
                auto remark = get_patch_remark(trans_json_path, prediction_id);

                if (remark == "TP") // 'remark' does not exist or marked as True Positive, changing to False Positive
                {
                    bool is_images_dir_created = FileUtils::createSubdirectory(AppPaths::Dataset_Path.string(), "images");
                    bool is_masks_dir_created = FileUtils::createSubdirectory(AppPaths::Dataset_Path.string(), "masks");
                    
                    if (!is_images_dir_created || !is_masks_dir_created)
                    {
                        throw std::runtime_error("Failed to create images or masks directory");
                    }

                    // Ensure the patch coordinates and dimensions are within bounds
                    if (position_x >= 0 && position_y >= 0 &&
                        position_x + patch_width <= m_image_pixbuf_report_original->get_width() &&
                        position_y + patch_height <= m_image_pixbuf_report_original->get_height()) {
                        
                        // Create a subpixbuf for the patch
                        auto patch_pixbuf = Gdk::Pixbuf::create_subpixbuf(m_image_pixbuf_report_original, position_x, position_y, patch_width, patch_height);

                        // Save the patch to a file
                        if (patch_pixbuf)
                        {
                            patch_pixbuf->save((AppPaths::Dataset_Path / "images" / filename).string(), "png");
                            
                            // Create a black mask pixbuf
                            auto mask_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, patch_width, patch_height);
                            mask_pixbuf->fill(0x000000);
                            
                            // Save the mask to a file
                            if (mask_pixbuf)
                            {
                                mask_pixbuf->save((AppPaths::Dataset_Path / "masks" / filename).string(), "png");
                            }
                        }

                        // Mark the patch as FP and update transaction json file
                        update_patch_remark(trans_json_path, prediction_id, "FP");
                        set_button_icon(delete_button, "/com/example/eagle_eye/confirm.svg");
                        update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 128);
                    }
                    else
                    {
                        std::cerr << "Patch coordinates are out of bounds!" << std::endl;
                    }
                }
                else if (remark == "FP") // Was False Positive, changing to True Positive.
                {
                    // Mark the patch as TP and update transaction json file
                    update_patch_remark(trans_json_path, prediction_id, "TP");

                    // Delete image/mask pair from Dataset path
                    auto imagePath = AppPaths::Dataset_Path / "images" / filename;
                    if (std::filesystem::exists(imagePath))
                    {
                        // Delete the image
                        std::filesystem::remove(imagePath);
                        std::cout << "File deleted: " << imagePath << std::endl;
                    }
                    else
                    {
                        std::cerr << "File not found: " << imagePath << std::endl;
                    }

                    auto maskPath = AppPaths::Dataset_Path / "masks" / filename;
                    if (std::filesystem::exists(maskPath))
                    {
                        // Delete the mask
                        std::filesystem::remove(maskPath);
                        std::cout << "File deleted: " << maskPath << std::endl;
                    }
                    else
                    {
                        std::cerr << "File not found: " << maskPath << std::endl;
                    }

                    set_button_icon(delete_button, "/com/example/eagle_eye/delete.svg");
                    update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 255);
                }
            }
            catch (const Glib::Exception& e)
            {
                std::cerr << "Error saving or deleting patch: " << e.what() << std::endl;
            }
        });
        action_box->pack_start(*delete_button, Gtk::PACK_SHRINK);

        // Add the button box to the item box
        item_box->pack_start(*action_box, Gtk::PACK_SHRINK);

        // Add the item box to the main box
        m_report_patches_box->pack_start(*item_box, Gtk::PACK_SHRINK);
    }

    m_report_patches_box->show_all_children();

    // Update mask pixel buf
    if (m_mask_pixbuf_report)
    {
        update_mask_color(m_mask_pixbuf_report);
        update_mask_alpha(m_mask_pixbuf_report, m_mask_alpha * 255);
    }

    // Queue the frame for display
    if (m_image_pixbuf_report)
    {
        m_report_image_display_area->set_size_request(frame_width, total_height);
        m_report_image_display_area->queue_draw();
    }
}

void MainWindow::load_detection_result_in_explorer(std::string &detection_result_folder)
{
    std::filesystem::path trans_json_path = std::filesystem::path(detection_result_folder) / "transaction_data.json";
    if (!std::filesystem::exists(trans_json_path))
    {
        std::cerr << "File not exists: transaction_data.json" << std::endl;
        return;
    }

    // Read the content of the JSON file
    std::ifstream json_file(trans_json_path);
    if (!json_file.is_open())
    {
        std::cerr << "Failed to open the file." << std::endl;
        return;
    }

    // Parse the JSON content
    nlohmann::json json_data;
    int patch_size;
    int frame_width;
    int frame_height;
    int num_frames;
    int total_height;

    try
    {
        json_file >> json_data;
        json_file.close();

        patch_size = json_data["patch_size"].get<int>();
        frame_width = json_data["frame_width"].get<int>();
        frame_height = json_data["frame_height"].get<int>();
        num_frames = json_data["num_frames"].get<int>();
        total_height = frame_height * num_frames;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return;
    }

    // Create the combined pixbuf for detection images.
    // Gdk::Pixbuf does not directly support a single-channel format, 
    // so still create an RGB pixbuf and replicate the grayscale values across the three color channels.
    m_image_pixbuf_explorer.reset();
    m_image_pixbuf_explorer = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    m_image_pixbuf_explorer->fill(0x00000000); // Fill with black

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
            m_image_pixbuf_explorer, 
            0, 
            current_y);

        pixbuf_image.reset();

        // Update the y-offset for the next image
        current_y += frame_height;
    }

    if (load_images_error)
    {
        // TODO, show a popup
        return;
    }

    // Create a transparent mask pixbuf of the same size as the image
    m_mask_pixbuf_explorer.reset();
    m_mask_pixbuf_explorer = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    // m_mask_pixbuf_explorer->fill(0xffffffbe); // For testing
    m_mask_pixbuf_explorer->fill(0x00000000); // Initialize the mask to be fully transparent black

    // Clear patches box before adding
    for (auto *child : m_detection_patches_box->get_children())
    {
        m_detection_patches_box->remove(*child);
    }

    // Load and position each prediction
    int predictions_per_row = frame_width / patch_size;
    if (frame_width % patch_size != 0)
    {
        predictions_per_row++; // Allow for an additional prediction if there's remaining space
    }

    // Preserve the original frame pixbuf
    m_image_pixbuf_explorer_original.reset();
    m_image_pixbuf_explorer_original = m_image_pixbuf_explorer->copy();
    // Extract transaction ID
    auto transaction_id = json_data["transaction_id"].get<std::string>();
    
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
        int patch_width = prediction_pixbuf->get_width();
        int patch_height = prediction_pixbuf->get_height();
        prediction_pixbuf->Gdk::Pixbuf::copy_area(
            0,
            0,
            patch_width,
            patch_height,
            m_mask_pixbuf_explorer,
            position_x,
            position_y
        );

        // Create a vertical box for each item
        auto item_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL);
        item_box->set_spacing(5); // Spacing between child elements

        // Create and add the title label
        auto label = Gtk::make_managed<Gtk::Label>("Patch ID: " + std::to_string(prediction_id));
        label->set_halign(Gtk::ALIGN_START); // Align text to the left
        item_box->pack_start(*label, Gtk::PACK_SHRINK);

        // Create and add the thumbnail
        const int thumbnail_width = 80;
        const int thumbnail_height = 80;
        auto thumbnail_pixbuf = prediction_pixbuf->scale_simple(
            thumbnail_width, 
            thumbnail_height, 
            Gdk::INTERP_BILINEAR);
        auto thumbnail = Gtk::make_managed<Gtk::Image>(thumbnail_pixbuf);
        thumbnail->set_halign(Gtk::ALIGN_START);
        item_box->pack_start(*thumbnail, Gtk::PACK_SHRINK);

        prediction_pixbuf.reset();

        // Create a horizontal box for buttons
        auto action_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
        action_box->set_spacing(10); // Spacing between buttons

        // Add "Focus" button
        auto view_button = Gtk::make_managed<Gtk::Button>();
        view_button->set_margin_top(5);
        view_button->signal_clicked().connect([this, label, position_x, position_y, patch_width, patch_height]() {
            try 
            {
                // Remove the "highlighted" class from the currently highlighted label
                if (m_current_selected_patch_in_explorer_lbl)
                {
                    m_current_selected_patch_in_explorer_lbl->get_style_context()->remove_class("highlighted");
                }

                // Highlight the new label
                auto style_context = label->get_style_context();
                style_context->add_class("highlighted");

                // Update the currently highlighted label
                m_current_selected_patch_in_explorer_lbl = label;

                // Create a Cairo surface based on the existing pixbuf
                auto surface = Cairo::ImageSurface::create(
                    Cairo::FORMAT_ARGB32,
                    m_image_pixbuf_explorer->get_width(),
                    m_image_pixbuf_explorer->get_height());
                auto cr = Cairo::Context::create(surface);

                // Clear existing drawings by re-rendering the original pixbuf
                Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_explorer_original, 0, 0); // Use the original pixbuf
                cr->paint();

                // Set the stroke color (e.g., red)
                cr->set_source_rgba(1.0, 0.0, 0.0, 1.0); // RGBA: red, fully opaque

                // Set line width for the rectangle edges
                cr->set_line_width(2.0);

                // Draw the edges of the rectangle
                cr->move_to(position_x, position_y); // Top-left corner
                cr->line_to(position_x + patch_width, position_y); // Top edge
                cr->line_to(position_x + patch_width, position_y + patch_height); // Right edge
                cr->line_to(position_x, position_y + patch_height); // Bottom edge
                cr->close_path(); // Close the rectangle (connect back to top-left)

                // Stroke the rectangle edges
                cr->stroke();

                // Update the pixbuf with the modified surface
                m_image_pixbuf_explorer = Gdk::Pixbuf::create(
                    surface, 0, 0,
                    surface->get_width(),
                    surface->get_height());

                // // Refresh the UI with the updated pixbuf
                m_detection_results_display_area->queue_draw();
            }
            catch (const Glib::Error& ex)
            {
                std::cerr << "Error drawing rectangle: " << ex.what() << std::endl;
            }
        });
        set_button_icon(view_button, "/com/example/eagle_eye/focus.svg");
        action_box->pack_start(*view_button, Gtk::PACK_SHRINK);

        // Add "Delete" button
        auto delete_button = Gtk::make_managed<Gtk::Button>();
        delete_button->set_margin_top(5);
        
        // Load button icon
        auto remark = prediction.value("remark", "TP");
        if (remark == "FP") // Marked as False Positive, the available action is to revert it back to True Positive.
        {
            set_button_icon(delete_button, "/com/example/eagle_eye/confirm.svg");
            update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 128);
        }
        else // 'remark' does not exist or was True Positive, the available action is to mark it as False Positive.
        {
            set_button_icon(delete_button, "/com/example/eagle_eye/delete.svg");
            update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 255);
        }

        delete_button->signal_clicked().connect([this, delete_button, trans_json_path, transaction_id, prediction_id, thumbnail, thumbnail_pixbuf, position_x, position_y, patch_width, patch_height]()
        {
            try
            {
                std::string filename = transaction_id + "_patch_" + std::to_string(prediction_id) + ".png";
                auto remark = get_patch_remark(trans_json_path, prediction_id);

                if (remark == "TP") // 'remark' does not exist or marked as True Positive, changing to False Positive
                {
                    bool is_images_dir_created = FileUtils::createSubdirectory(AppPaths::Dataset_Path.string(), "images");
                    bool is_masks_dir_created = FileUtils::createSubdirectory(AppPaths::Dataset_Path.string(), "masks");
                    
                    if (!is_images_dir_created || !is_masks_dir_created)
                    {
                        throw std::runtime_error("Failed to create images or masks directory");
                    }

                    // Ensure the patch coordinates and dimensions are within bounds
                    if (position_x >= 0 && position_y >= 0 &&
                        position_x + patch_width <= m_image_pixbuf_explorer_original->get_width() &&
                        position_y + patch_height <= m_image_pixbuf_explorer_original->get_height()) {
                        
                        // Create a subpixbuf for the patch
                        auto patch_pixbuf = Gdk::Pixbuf::create_subpixbuf(m_image_pixbuf_explorer_original, position_x, position_y, patch_width, patch_height);

                        // Save the patch to a file
                        if (patch_pixbuf)
                        {
                            patch_pixbuf->save((AppPaths::Dataset_Path / "images" / filename).string(), "png");
                            
                            // Create a black mask pixbuf
                            auto mask_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, patch_width, patch_height);
                            mask_pixbuf->fill(0x000000);
                            
                            // Save the mask to a file
                            if (mask_pixbuf)
                            {
                                mask_pixbuf->save((AppPaths::Dataset_Path / "masks" / filename).string(), "png");
                            }
                        }

                        // Mark the patch as FP and update transaction json file
                        update_patch_remark(trans_json_path, prediction_id, "FP");
                        set_button_icon(delete_button, "/com/example/eagle_eye/confirm.svg");
                        update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 128);
                    }
                    else
                    {
                        std::cerr << "Patch coordinates are out of bounds!" << std::endl;
                    }
                }
                else if (remark == "FP") // Was False Positive, changing to True Positive.
                {
                    // Mark the patch as TP and update transaction json file
                    update_patch_remark(trans_json_path, prediction_id, "TP");

                    // Delete image/mask pair from Dataset path
                    auto imagePath = AppPaths::Dataset_Path / "images" / filename;
                    if (std::filesystem::exists(imagePath))
                    {
                        // Delete the image
                        std::filesystem::remove(imagePath);
                        std::cout << "File deleted: " << imagePath << std::endl;
                    }
                    else
                    {
                        std::cerr << "File not found: " << imagePath << std::endl;
                    }

                    auto maskPath = AppPaths::Dataset_Path / "masks" / filename;
                    if (std::filesystem::exists(maskPath))
                    {
                        // Delete the mask
                        std::filesystem::remove(maskPath);
                        std::cout << "File deleted: " << maskPath << std::endl;
                    }
                    else
                    {
                        std::cerr << "File not found: " << maskPath << std::endl;
                    }

                    set_button_icon(delete_button, "/com/example/eagle_eye/delete.svg");
                    update_patch_thumbnail_alpha(thumbnail_pixbuf, thumbnail, 255);
                }
            }
            catch (const Glib::Exception& e)
            {
                std::cerr << "Error saving or deleting patch: " << e.what() << std::endl;
            }
        });
        action_box->pack_start(*delete_button, Gtk::PACK_SHRINK);

        // Add the button box to the item box
        item_box->pack_start(*action_box, Gtk::PACK_SHRINK);

        // Add the item box to the main box
        m_detection_patches_box->pack_start(*item_box, Gtk::PACK_SHRINK);
    }

    m_detection_patches_box->show_all_children();

    // Update mask pixel buf
    if (m_mask_pixbuf_explorer)
    {
        update_mask_color(m_mask_pixbuf_explorer);
        update_mask_alpha(m_mask_pixbuf_explorer, m_mask_alpha * 255);
    }

    // Queue the frame for display
    if (m_image_pixbuf_explorer)
    {
        m_detection_results_display_area->set_size_request(frame_width, total_height);
        m_detection_results_display_area->queue_draw();
    }
}

void MainWindow::update_patch_thumbnail_alpha(Glib::RefPtr<Gdk::Pixbuf> thumbnail_pixbuf, Gtk::Image *thumbnail, int alpha_value)
{
    // Get the current width and height of the thumbnail
    const int thumbnail_width = thumbnail_pixbuf->get_width();
    const int thumbnail_height = thumbnail_pixbuf->get_height();

    // Create a copy of the Pixbuf with an alpha channel (RGBA format)
    auto alpha_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, thumbnail_width, thumbnail_height);

    // Get the pixel data of the original and the new alpha Pixbuf
    guchar* original_data = thumbnail_pixbuf->get_pixels();
    guchar* alpha_data = alpha_pixbuf->get_pixels();

    // Loop through each pixel to copy the RGB values and modify the alpha
    for (int y = 0; y < thumbnail_height; ++y) {
        for (int x = 0; x < thumbnail_width; ++x) {
            int i = (y * thumbnail_width + x) * RGB_CHANNELS;
            int j = (y * thumbnail_width + x) * RGBA_CHANNELS;
            // Copy the RGB values from the original Pixbuf
            alpha_data[j] = original_data[i];         // Red
            alpha_data[j + 1] = original_data[i + 1]; // Green
            alpha_data[j + 2] = original_data[i + 2]; // Blue
            alpha_data[j + 3] = alpha_value;          // Set the alpha (opacity)
        }
    }

    thumbnail->set(alpha_pixbuf);
}

std::string MainWindow::get_patch_remark(const std::string &transaction_json_path, int prediction_id)
{
    nlohmann::json json_data;
    std::string remark_default = "TP";

    try
    {
        // Open the JSON file for reading
        std::ifstream json_file(transaction_json_path);
        if (!json_file.is_open())
        {
            throw std::ios_base::failure("Failed to open JSON file.");
        }

        // Parse the JSON content
        json_file >> json_data;
        json_file.close();

        // Update the JSON
        if (json_data.contains("predictions") && json_data["predictions"].is_array())
        {
            for (auto &prediction : json_data["predictions"])
            {
                if (prediction.contains("prediction_id") && prediction["prediction_id"] == prediction_id)
                {
                    auto remark = prediction.value("remark", remark_default);
                    return remark;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return remark_default;
}

void MainWindow::update_patch_remark(const std::string &transaction_json_path, int prediction_id, const std::string &remark)
{
    nlohmann::json json_data;

    try
    {
        // Open the JSON file for reading
        std::ifstream json_file(transaction_json_path);
        if (!json_file.is_open())
        {
            throw std::ios_base::failure("Failed to open JSON file.");
        }

        // Parse the JSON content
        json_file >> json_data;
        json_file.close();

        // Update the JSON
        if (json_data.contains("predictions") && json_data["predictions"].is_array())
        {
            for (auto &prediction : json_data["predictions"])
            {
                if (prediction.contains("prediction_id") && prediction["prediction_id"] == prediction_id)
                {
                    prediction["remark"] = remark; // Add or update the remark field
                    std::cout << "Remark added to prediction_id " << prediction_id << std::endl;
                }
            }
        }

        // Write the updated JSON back to the file
        std::ofstream output_file(transaction_json_path);
        if (!output_file.is_open())
        {
            throw std::ios_base::failure("Failed to open JSON file for writing.");
        }
        output_file << json_data.dump(4); // Pretty-print with 4 spaces
        output_file.close();

        std::cout << "JSON file updated successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void MainWindow::on_enable_masking_changed()
{
    m_show_mask_detection_result = m_detection_results_masking_switch->get_active();
    Gtk::ListBoxRow* selected_row = m_detection_results_listbox->get_selected_row();
    if (selected_row)
    {
        on_detection_result_selected(selected_row);
    }
}

bool MainWindow::on_detection_results_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x_detection, m_offset_y_detection);   // Apply panning offset
    cr->scale(m_zoom_factor_detection, m_zoom_factor_detection); // Apply zoom

    // Draw the images
    if (m_image_pixbuf_explorer)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_explorer, 0, 0);
        cr->paint();
    }

    // Draw the masks
    if (m_show_mask_detection_result && m_mask_pixbuf_explorer)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_mask_pixbuf_explorer, 0, 0);
        cr->paint();
    }

    return true;
}

void MainWindow::load_detection_settings()
{
    std::string detection_camera;
    auto detection_rate = 0;
    std::string digital_io_type;
    std::string digital_input;
    std::string digital_input_line_number;
    std::string digital_output;
    std::string digital_output_line_number;
    auto confidence_threshold = 0.5;
    auto pixel_threshold = 0.1;
    auto max_per_day = 1000;
    auto days_to_retain = 30;

    auto detection_settings = SettingsService::get_settings("detection");
    if (!detection_settings.empty())
    {
        if (detection_settings.contains("detection_camera"))
        {
            detection_camera = detection_settings["detection_camera"];
        }

        if (detection_settings.contains("detection_rate"))
        {
            detection_rate = detection_settings["detection_rate"];
        }

        if (detection_settings.contains("digital_input"))
        {
            digital_input = detection_settings["digital_input"];
        }

        if (detection_settings.contains("digital_output"))
        {
            digital_output = detection_settings["digital_output"];
        }

        if (detection_settings.contains("confidence_threshold"))
        {
            confidence_threshold = detection_settings["confidence_threshold"];
        }

        if (detection_settings.contains("pixel_threshold"))
        {
            pixel_threshold = detection_settings["pixel_threshold"];
        }

        if (detection_settings.contains("max_per_day"))
        {
            max_per_day = detection_settings["max_per_day"];
        }

        if (detection_settings.contains("days_to_retain"))
        {
            days_to_retain = detection_settings["days_to_retain"];
        }
    }

    if (digital_input != "")
    {
        auto cam_settings = SettingsService::get_settings(digital_input);

        if (!cam_settings.empty() && cam_settings.contains("digital_io_type"))
        {
            digital_io_type = cam_settings["digital_io_type"];
            if (digital_io_type == "Input")
            {
                if (cam_settings.contains("digital_input_line_number"))
                {
                    digital_input_line_number = cam_settings["digital_input_line_number"];
                }
            }
            else
            {
                // Digital IO type mismatch, reset digital_input to empty string
                digital_input = "";
            }
        }
    }

    if (digital_output != "")
    {
        auto cam_settings = SettingsService::get_settings(digital_output);

        if (!cam_settings.empty() && cam_settings.contains("digital_io_type"))
        {
            digital_io_type = cam_settings["digital_io_type"];
            if (digital_io_type == "Output")
            {
                if (cam_settings.contains("digital_output_line_number"))
                {
                    digital_output_line_number = cam_settings["digital_output_line_number"];
                }
            }
            else
            {
                // Digital IO type mismatch, reset digital_output to empty string
                digital_output = "";
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
}

void MainWindow::on_cancel_detection_settings_clicked()
{
    load_detection_settings();
}

void MainWindow::on_save_detection_settings_clicked()
{
    // Build settings content
    nlohmann::json new_settings;

    if (m_select_detection_camera_cbox)
    {
        auto detection_camera = m_select_detection_camera_cbox->get_active_text();
        m_detection_camera_lbl->set_text(detection_camera);
        new_settings["detection_camera"] = detection_camera;
    }
    if (m_detection_rate_sb)
    {
        auto detection_rate = m_detection_rate_sb->get_value_as_int();
        new_settings["detection_rate"] = detection_rate;
        m_detection_rate_lbl->set_text(std::to_string(detection_rate));
    }
    if (m_select_detection_digital_input_cbox)
    {
        auto digital_input = m_select_detection_digital_input_cbox->get_active_text();
        new_settings["digital_input"] = digital_input;
    }
    if (m_select_detection_digital_output_cbox)
    {
        auto digital_output = m_select_detection_digital_output_cbox->get_active_text();
        new_settings["digital_output"] = digital_output;
    }
    if (m_detection_sensitivity_scale)
    {
        auto confidence_threshold = m_detection_sensitivity_scale->get_value();
        new_settings["confidence_threshold"] = confidence_threshold;
    }
    if (m_anomaly_size_threshold_scale)
    {
        auto pixel_threshold = m_anomaly_size_threshold_scale->get_value();
        new_settings["pixel_threshold"] = pixel_threshold;
    }

    // Save detection settings
    SettingsService::add_or_update_settings("detection", new_settings);
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

void MainWindow::on_digital_input_line_number_changed()
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        std::string selected_line_number = m_settings_digital_input_line_number_cbox->get_active_text();
        if (selected_line_number != "")
        {
            int nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", selected_line_number.c_str());
            if (nRet == MV_OK)
            {
                // Wait a bit or Network error occurs - MV_E_NETER (0x80000206)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Update Input Event Trigger
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
                            std::cout << "Digital input event trigger: " << trigger << std::endl;
                            // Only add Digital Input event triggers
                            if (trigger == "Line0RisingEdge" || trigger == "Line0FallingEdge")
                            {
                                m_settings_digital_input_event_trigger_cbox->append(trigger);
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
                    std::cerr << "Failed to get event_trigger. Error code: " << nRet << std::endl;
                }

                // Update Digital Input Event Notification
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
                        }
                        else
                        {
                            std::cerr << "Failed to get symbolic name for entry " << i << ". Error code: " << nRet << std::endl;
                        }
                    }
                }
                else
                {
                    std::cerr << "Failed to get notification_status. Error code: " << nRet << std::endl;
                }
            }
        }
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

void MainWindow::on_strobe_enable_state_set()
{
    auto sn = m_sn_lbl->get_text();
    if (m_connected_device_handles.find(sn) != m_connected_device_handles.end())
    {
        void *device_handle = m_connected_device_handles[sn];
        bool state = m_settings_strobe_enable_switch->get_active();
        int nRet = MV_CC_SetBoolValue(device_handle, "StrobeEnable", state);
        if (nRet != MV_OK)
        {
            std::cerr << "Error to set StrobeEnable to " << state << " Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetBoolValue(StrobeEnable). Error code: " + std::to_string(nRet), Logger::ERROR);
        }
    }
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
        m_logger->log("Trigger digital output via software succeeded");
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
    // Check if height is a multiple of patch size (256)
    size_t height = m_height_sb->get_value_as_int();
    if (height <= 0 || height % 256 != 0)
    {
        // Create the message dialog with the specified parent window, message text, and button options
        Gtk::MessageDialog dialog(*this, 
                                "Height must be a multiple of 256", 
                                false,
                                Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK,
                                true);

        // Run the dialog and wait for the user to press the OK button
        dialog.run();
        return;
    }

    std::string serial_number;
    if (m_sn_lbl)
    {
        serial_number = m_sn_lbl->get_text();
    }
    std::string section_name = serial_number;
    nlohmann::json new_settings;
    std::vector<std::string> settings_to_remove;
    
    // Build settings content
    if (m_exposure_time_entry)
    {
        try 
        {
            // Convert string to float and store in JSON
            new_settings["exposure_time"] = std::stof(m_exposure_time_entry->get_text());
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error converting exposure_time to float: " << e.what() << std::endl;
        }
    }
    if (m_width_sb)
    {
        new_settings["width"] = m_width_sb->get_value_as_int();
    }
    if (m_height_sb)
    {
        new_settings["height"] = m_height_sb->get_value_as_int();
    }
    if (m_offset_x_sb)
    {
        new_settings["offset_x"] = m_offset_x_sb->get_value_as_int();
    }
    if (m_offset_y_sb)
    {
        new_settings["offset_y"] = m_offset_y_sb->get_value_as_int();
    }
    if (m_digital_io_type_cbox)
    {
        std::string digital_io_type = m_digital_io_type_cbox->get_active_text();
        new_settings["digital_io_type"] = digital_io_type;

        if (digital_io_type == "Input")
        {
            // Remove digital output settings
            settings_to_remove.push_back("digital_output_line_number");
            settings_to_remove.push_back("digital_output_line_mode");
            settings_to_remove.push_back("digital_output_line_source");
            settings_to_remove.push_back("digital_output_strobe_enable");
            settings_to_remove.push_back("digital_output_strobe_duration");

            if (m_settings_digital_input_line_number_cbox)
            {
                new_settings["digital_input_line_number"] = m_settings_digital_input_line_number_cbox->get_active_text();
            }
            if (m_settings_digital_input_debouncer_time_sb)
            {
                new_settings["digital_input_debouncer_time"] = m_settings_digital_input_debouncer_time_sb->get_value_as_int();
            }
            if (m_settings_digital_input_event_trigger_cbox)
            {
                new_settings["digital_input_event_trigger"] = m_settings_digital_input_event_trigger_cbox->get_active_text();
            }
            if (m_settings_digital_input_notification_status_cbox)
            {
                new_settings["digital_input_notification_status"] = m_settings_digital_input_notification_status_cbox->get_active_text();
            }
        }
        else if (digital_io_type == "Output")
        {
            std::cout << new_settings.dump(4) << std::endl;

            // Remove digital input settings
            settings_to_remove.push_back("digital_input_line_number");
            settings_to_remove.push_back("digital_input_debouncer_time");
            settings_to_remove.push_back("digital_input_event_trigger");
            settings_to_remove.push_back("digital_input_notification_status");

            std::cout << new_settings.dump(4) << std::endl;
            
            if (m_settings_digital_output_line_number_cbox)
            {
                new_settings["digital_output_line_number"] = m_settings_digital_output_line_number_cbox->get_active_text();
            }
            if (m_settings_digital_output_line_mode_cbox)
            {
                new_settings["digital_output_line_mode"] = m_settings_digital_output_line_mode_cbox->get_active_text();
            }
            if (m_settings_digital_output_line_source_cbox)
            {
                new_settings["digital_output_line_source"] = m_settings_digital_output_line_source_cbox->get_active_text();
            }
            if (m_settings_strobe_enable_switch)
            {
                new_settings["digital_output_strobe_enable"] = m_settings_strobe_enable_switch->get_active();
            }
            if (m_settings_strobe_duration_sb)
            {
                new_settings["digital_output_strobe_duration"] = m_settings_strobe_duration_sb->get_value_as_int();
            }
        }
    }

    std::cout << new_settings.dump(4) << std::endl;

    // Save detection settings
    SettingsService::remove_settings(section_name, settings_to_remove);
    SettingsService::add_or_update_settings(section_name, new_settings);
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

    auto cam_settings = SettingsService::get_settings(serial_number);

    // Load digital IO type
    std::string digital_io_type = "";
    if (!cam_settings.empty() && cam_settings.contains("digital_io_type"))
    {
        digital_io_type = cam_settings["digital_io_type"];
        if (m_digital_io_type_cbox)
        {
            m_digital_io_type_cbox->set_active_text(digital_io_type);
        }
    }

    // Load digital Input line number
    std::string digital_input_line_number = "";
    if (!cam_settings.empty() && cam_settings.contains("digital_input_line_number"))
    {
        digital_input_line_number = cam_settings["digital_input_line_number"];
    }

    // Digital Input Line Number (fixed to "Line0")
    m_settings_digital_input_line_number_cbox->remove_all();
    m_settings_digital_input_line_number_cbox->append("Line0");
    m_settings_digital_input_line_number_cbox->set_active_text(digital_input_line_number);

    if (digital_input_line_number != "")
    {
        nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", digital_input_line_number.c_str());
        if (nRet == MV_OK)
        {
            // Wait a bit or Network error occurs - MV_E_NETER (0x80000206)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Digital Input Debounce Time
            MVCC_INTVALUE debounce_time = {0};
            nRet = MV_CC_GetIntValue(device_handle, "LineDebouncerTime", &debounce_time);
            if (nRet == MV_OK && m_debounce_time_adj && m_settings_digital_input_debouncer_time_sb)
            {
                m_debounce_time_adj->set_lower(debounce_time.nMin);
                m_debounce_time_adj->set_upper(debounce_time.nMax);
                m_debounce_time_adj->set_step_increment(debounce_time.nInc);
                m_settings_digital_input_debouncer_time_sb->set_value(debounce_time.nCurValue);
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
                        std::cout << "Digital input event trigger: " << trigger << std::endl;
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
        }
    }

    // Load digital Output line number from .ini file
    std::string digital_output_line_number = "";
    if (!cam_settings.empty() && cam_settings.contains("digital_output_line_number"))
    {
        digital_output_line_number = cam_settings["digital_output_line_number"];
    }

    // Digital Output Line Number (fixed to "Line1" and "Line2")
    m_settings_digital_output_line_number_cbox->remove_all();
    m_settings_digital_output_line_number_cbox->append("Line1");
    m_settings_digital_output_line_number_cbox->append("Line2");
    m_settings_digital_output_line_number_cbox->set_active_text(digital_output_line_number);

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

    load_recent_projects();
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

        nlohmann::json response_json = nlohmann::json::parse(message);
        std::string res_trans_id = response_json["transaction_id"];
        if (res_trans_id != m_trans_id)
        {
            std::cout << "Transaction ID mismatch" << std::endl;
            return;
        }

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
    
    auto cam_settings = SettingsService::get_settings(sn);
    if (!cam_settings.empty())
    {
        if (cam_settings.contains("exposure_time"))
        {
            float exposure_time = cam_settings["exposure_time"];
            nRet = MV_CC_SetFloatValue(device_handle, "ExposureTime", exposure_time);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set exposure time. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetFloatValue(ExposureTime): " + std::to_string(nRet), Logger::ERROR);
            }
        }

        if (cam_settings.contains("width"))
        {
            int width = cam_settings["width"];
            nRet = MV_CC_SetIntValue(device_handle, "Width", width);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set width. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetIntValue(Width): " + std::to_string(nRet), Logger::ERROR);
            }
        }

        if (cam_settings.contains("height"))
        {
            int height = cam_settings["height"];
            nRet = MV_CC_SetIntValue(device_handle, "Height", height);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set height. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetIntValue(Height): " + std::to_string(nRet), Logger::ERROR);
            }
        }

        if (cam_settings.contains("offset_x"))
        {
            int offset_x = cam_settings["offset_x"];
            nRet = MV_CC_SetIntValue(device_handle, "OffsetX", offset_x);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set offsetX. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetIntValue(OffsetX): " + std::to_string(nRet), Logger::ERROR);
            }
        }

        if (cam_settings.contains("offset_y"))
        {
            int offset_y = cam_settings["offset_y"];
            nRet = MV_CC_SetIntValue(device_handle, "OffsetY", offset_y);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set offsetY. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetIntValue(OffsetY): " + std::to_string(nRet), Logger::ERROR);
            }
        }

        // LineSelector needs to be set before accessing any digital IO settings,
        // otherwise MV_E_GC_ACCESS (0x80000106) occurs.
        // So "digital_input_line_number=*" line must be placed before any "digital_input_<setting>=*" line,
        // Same thing for digital output.
        if (cam_settings.contains("digital_input_line_number"))
        {
            std::string line_number = cam_settings["digital_input_line_number"];
            nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", line_number.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set LineSelector. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValueByString(LineSelector): " + std::to_string(nRet), Logger::ERROR);
            }    
        }
        
        if (cam_settings.contains("digital_input_debouncer_time"))
        {
            int debouncer_time = cam_settings["digital_input_debouncer_time"];
            nRet = MV_CC_SetIntValue(device_handle, "LineDebouncerTime", debouncer_time);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set LineDebouncerTime. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetIntValue(LineDebouncerTime): " + std::to_string(nRet), Logger::ERROR);
            }
        }
        
        if (cam_settings.contains("digital_input_event_trigger"))
        {
            std::string event_trigger = cam_settings["digital_input_event_trigger"];
            nRet = MV_CC_SetEnumValueByString(device_handle, "EventSelector", event_trigger.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set EventSelector. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValueByString(EventSelector): " + std::to_string(nRet), Logger::ERROR);
            }                        
        }
        if (cam_settings.contains("digital_input_notification_status"))
        {
            std::string notification_status = cam_settings["digital_input_notification_status"];
            nRet = MV_CC_SetEnumValueByString(device_handle, "EventNotification", notification_status.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set EventNotification. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValueByString(EventNotification): " + std::to_string(nRet), Logger::ERROR);
            }                         
        }

        if (cam_settings.contains("digital_output_line_number"))
        {
            std::string line_number = cam_settings["digital_output_line_number"];
            nRet = MV_CC_SetEnumValueByString(device_handle, "LineSelector", line_number.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set LineSelector. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValueByString(LineSelector): " + std::to_string(nRet), Logger::ERROR);
            }
        }

        if (cam_settings.contains("digital_output_line_mode"))
        {
            std::string line_mode = cam_settings["digital_output_line_mode"];
            nRet = MV_CC_SetEnumValueByString(device_handle, "LineMode", line_mode.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set LineMode. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValueByString(LineMode): " + std::to_string(nRet), Logger::ERROR);
            }
        }
        
        if (cam_settings.contains("digital_output_line_source"))
        {
            std::string line_source = cam_settings["digital_output_line_source"];
            nRet = MV_CC_SetEnumValueByString(device_handle, "LineSource", line_source.c_str());
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set LineSource. Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetEnumValueByString(LineSource): " + std::to_string(nRet), Logger::ERROR);
            }
        }
        
        if (cam_settings.contains("digital_output_strobe_enable"))
        {
            bool strobe_enabled = cam_settings["digital_output_strobe_enable"];
            nRet = MV_CC_SetBoolValue(device_handle, "StrobeEnable", strobe_enabled);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set StrobeEnable to " << strobe_enabled << " Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetBoolValue(StrobeEnable). Error code: " + std::to_string(nRet), Logger::ERROR);
            }
        }
        
        if (cam_settings.contains("digital_output_strobe_duration"))
        {
            int strobe_duration = cam_settings["digital_output_strobe_duration"];
            nRet = MV_CC_SetIntValue(device_handle, "StrobeLineDuration", strobe_duration);
            if (nRet != MV_OK)
            {
                std::cerr << "Error to set StrobeLineDuration to " << strobe_duration << " Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_SetIntValue(StrobeLineDuration). Error code: " + std::to_string(nRet), Logger::ERROR);
            }
        }
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
    if (m_startup_btn->get_active())
    {
        m_content_stack->set_visible_child("page_startup");
    }
    else if (m_runtime_btn->get_active())
    {
        m_content_stack->set_visible_child("page_runtime");
    }
    else if (m_toolkit_btn->get_active())
    {
        m_content_stack->set_visible_child("page_toolkit");
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

void MainWindow::on_new_project_clicked()
{
    if (!m_connected_device_handles.empty())
    {
        show_camera_connect_warning(*this, "The camera(s) appears to be in use. Please disconnect it before proceeding.");
        return;
    }

    // Load the dialog from Glade
    Gtk::Dialog* dialog = nullptr;
    m_builder->get_widget("new_project_dialog", dialog);
    
    Gtk::Entry* project_name_entry = nullptr;
    m_builder->get_widget("project_name_entry", project_name_entry);

    // Populate a default project name
    if (project_name_entry)
    {
        project_name_entry->set_text("detection_project");
    }

    if (dialog)
    {
        // Run the dialog
        int response = dialog->run();

        if (response == Gtk::ResponseType::RESPONSE_OK)
        {
            if (project_name_entry)
            {
                Glib::ustring project_name = project_name_entry->get_text();
                if (!project_name.empty())
                {
                    if (create_project(project_name))
                    {
                        // Reset session times when a new project is created
                        m_session_times.clear();
                        
                        // Clear transactions list
                        for (auto *child : m_report_transactions_listbox->get_children())
                        {
                            m_report_transactions_listbox->remove(*child);
                        }

                        // Clear display area on report page
                        m_image_pixbuf_report.reset();
                        m_mask_pixbuf_report.reset();
                        m_report_image_display_area->queue_draw();
                        
                        // Clear patches box before adding
                        for (auto *child : m_report_patches_box->get_children())
                        {
                            m_report_patches_box->remove(*child);
                        }
                        
                        // Clear position track by redrawing
                        m_report_position_display_area->queue_draw();

                        // Update main window title with a project name
                        set_window_title(APP_NAME + " - " + project_name);
                        
                        // Update runtime page
                        update_runtime_page("create_project");

                        // Navigate to runtime page
                        m_runtime_btn->set_active(true);
                        
                        // Update recent projects list
                        load_recent_projects();
                    }
                }
                else
                {
                    std::cout << "No project name provided." << std::endl;
                }
            }
        }
        else
        {
            std::cout << "Dialog canceled." << std::endl;
        }

        if (project_name_entry)
        {
            project_name_entry->set_text("");
        }

        // Hide the dialog after use
        dialog->hide();
    }
}

bool MainWindow::create_project(const std::string &project_name)
{
    // Create a project file (*.dscanproj) within a folder
    try
    {
        // Create the directory path
        std::filesystem::path project_folder = AppPaths::Project_Path(project_name);
        std::filesystem::create_directories(project_folder);

        // Create the JSON object with project name and last modified time
        nlohmann::json project_json;
        project_json["project_name"] = project_name;
        auto now = std::chrono::system_clock::now();
        project_json["last_modified_time"] = TimeUtils::get_formatted_time(now);
        
        // Generate the project file path
        std::filesystem::path project_file = project_folder / (project_name + ".dscanproj");

        // Write JSON to the file
        std::ofstream ofs(project_file);
        if (!ofs.is_open())
        {
            std::cerr << "Failed to open file: " << project_file << std::endl;
            return false;
        }
        ofs << project_json.dump(4); // Pretty-print with 4 spaces
        ofs.close();

        std::cout << "Project created successfully: " << project_file << std::endl;
        m_curr_project_name = project_name;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating project: " << e.what() << std::endl;
        return false;
    }
}

void MainWindow::on_open_project_clicked()
{
    if (!m_connected_device_handles.empty())
    {
        show_camera_connect_warning(*this, "The camera(s) appears to be in use. Please disconnect it before proceeding.");
        return;
    }

    // Create a FileChooserDialog in Open mode
    Gtk::FileChooserDialog dialog("Open Project", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);

    // Set the initial folder
    dialog.set_current_folder(AppPaths::Projects_Path);

    // Add a filter to show only CSV files
    auto ds_filter = Gtk::FileFilter::create();
    ds_filter->set_name("project files");
    ds_filter->add_pattern("*.dscanproj");
    dialog.add_filter(ds_filter);

    // Add buttons for user actions
    dialog.add_button("_Cancel", Gtk::ResponseType::RESPONSE_REJECT);
    dialog.add_button("_Open", Gtk::ResponseType::RESPONSE_ACCEPT);

    // Show the dialog and wait for user response
    int result = dialog.run();

    switch (result)
    {
        case Gtk::ResponseType::RESPONSE_ACCEPT:
        {
            // Get the selected file path
            std::string file_path = dialog.get_filename();
            std::cout << "File selected to open: " << file_path << std::endl;
            open_project(file_path);
            break;
        }
        case Gtk::ResponseType::RESPONSE_REJECT:
            std::cout << "Save operation canceled." << std::endl;
            break;

        default:
            std::cout << "Unexpected response." << std::endl;
            break;
    } 
}

void MainWindow::open_project(const std::string &project_file_path)
{
    // Get project name from *.dscanproj file and update current project member
    auto project_json_optional = FileUtils::get_json(project_file_path);
    if (project_json_optional.has_value())
    {
        auto &project_json = project_json_optional.value();
        if (project_json.contains("project_name"))
        {
            m_curr_project_name = project_json["project_name"];

            // Update main window title with a project name
            set_window_title(APP_NAME + " - " + m_curr_project_name);

            // Clear transactions list
            for (auto *child : m_report_transactions_listbox->get_children())
            {
                m_report_transactions_listbox->remove(*child);
            }

            // Clear display area on report page
            m_image_pixbuf_report.reset();
            m_mask_pixbuf_report.reset();
            m_report_image_display_area->queue_draw();
            
            // Clear patches box before adding
            for (auto *child : m_report_patches_box->get_children())
            {
                m_report_patches_box->remove(*child);
            }
            
            // Clear position track by redrawing
            m_report_position_display_area->queue_draw();

            // Update runtime page
            update_runtime_page("open_project");

            // Navigate to runtime page
            m_runtime_btn->set_active(true);

            // Reconstruct session times when opening a project
            m_session_times.clear();  
            if (project_json.contains("session_times")) 
            {
                for (const auto& session : project_json["session_times"])
                {
                    auto start_time = TimeUtils::parse_time(session["start_time"]);
                    auto end_time = (session["end_time"] == "max")
                                    ? std::chrono::system_clock::time_point::max()
                                    : TimeUtils::parse_time(session["end_time"]);

                    m_session_times.emplace_back(start_time, end_time);
                }
            }
        }

        auto now = std::chrono::system_clock::now();
        project_json["last_modified_time"] = TimeUtils::get_formatted_time(now);
        // Write JSON to the file
        std::ofstream ofs(project_file_path);
        if (ofs.is_open())
        {
            ofs << project_json.dump(4); // Pretty-print with 4 spaces
            ofs.close();
        }
        else
        {
            std::cerr << "Failed to open file: " << project_file_path << std::endl;
        }

        // Update recent projects list
        load_recent_projects();
    }
}

void MainWindow::on_quick_start_clicked()
{
    if (!m_connected_device_handles.empty())
    {
        show_camera_connect_warning(*this, "The camera(s) appears to be in use. Please disconnect it before proceeding.");
        return;
    }

    // Reset session times when a new project is created
    m_session_times.clear();

    // Clear transactions list
    for (auto *child : m_report_transactions_listbox->get_children())
    {
        m_report_transactions_listbox->remove(*child);
    }

    // Clear display area on report page
    m_image_pixbuf_report.reset();
    m_mask_pixbuf_report.reset();
    m_report_image_display_area->queue_draw();

    // Clear patches box before adding
    for (auto *child : m_report_patches_box->get_children())
    {
        m_report_patches_box->remove(*child);
    }

    // Clear position track by redrawing
    m_report_position_display_area->queue_draw();

    // Update main window title with a project name
    set_window_title(APP_NAME);

    // Update runtime page
    update_runtime_page("quick_start");

    // Navigate to runtime page
    m_runtime_btn->set_active(true);  
}

void MainWindow::update_runtime_page(const std::string &mode)
{
    // Hide the no project text
    if (m_runtime_no_project_lbl)
    {
        m_runtime_no_project_lbl->set_visible(false);
    }

    // Make all runtime nav buttons visible
    if (m_runtime_nav_button_box)
    {
        m_runtime_nav_button_box->set_visible(true);

        for (auto * child : m_runtime_nav_button_box->get_children())
        {
            child->set_visible(true);
        }
    }

    // Make all children of the runtime stack visible
    if (m_runtime_stack)
    {
        m_runtime_stack->set_visible(true);

        for (auto* child : m_runtime_stack->get_children())
        {
            child->set_visible(true);
        }
    }

    // Populate the runtime page base on the given mode
    if (mode == "create_project" || mode == "open_project")
    {
        if (m_runtime_control_panel_rbtn)
        {
            m_runtime_control_panel_rbtn->set_active(true);
        }
    }
    else if (mode == "quick_start")
    {
        if (m_runtime_report_rbtn)
        {
            m_runtime_report_rbtn->set_visible(false);
        }
        if (m_runtime_control_panel_rbtn)
        {
            m_runtime_control_panel_rbtn->set_active(true);
        }
    }
}

void MainWindow::on_recent_project_selected(Gtk::ListBoxRow* row)
{
    if (!m_connected_device_handles.empty())
    {
        show_camera_connect_warning(*this, "The camera(s) appears to be in use. Please disconnect it before proceeding.");
        return;
    }

    if (row)
    {
        auto row_box = dynamic_cast<Gtk::Box*>(row->get_child());
        if (row_box)
        {
            std::string project_path_str = row_box->get_tooltip_text();

            auto project_label = dynamic_cast<Gtk::Label *>(row_box->get_children()[0]);
            if (project_label)
            {
                std::string project_name = project_label->get_text();
                if (project_name == "more...")
                {
                    on_open_project_clicked();
                }
                else
                {
                    std::filesystem::path project_path(project_path_str);
                    auto project_file_path = project_path / (project_name + ".dscanproj");
                    open_project(project_file_path.string());         
                }
            }
        }
    }
    else
    {
        std::cout << "No row selected!" << std::endl;
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
    size_t buffer = 2448 * 2048 * MAX_FRAME_BATCH_SIZE;

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

    auto frame_width = stImageInfo.nWidth;
    auto frame_height = stImageInfo.nHeight;
    auto frame_size = frame_width * frame_height;
    std::vector<FrameOffsetInfo> frame_offsets;

    // Save the frame metadata
    frame_offsets.push_back({ 0, frame_size, sn });

    // Calculate the memory address to copy this frame
    void* frame_ptr = static_cast<uint8_t*>(shm_ptr);

    // Copy the frame data into the calculated memory location
    std::memcpy(frame_ptr, pData, frame_size);

    if (!frame_offsets.empty())
    {
        // Send the command to the ws server
        m_trans_id = generate_transaction_id();

        // Get confidence threshold and pixel threshold from settings file
        double confidence_threshold = 0.5;
        double pixel_threshold = 0.1;

        auto detection_settings = SettingsService::get_settings("detection");
        if (!detection_settings.empty())
        {
            confidence_threshold = detection_settings["confidence_threshold"];
            pixel_threshold = detection_settings["pixel_threshold"];
        }

        // Get current Datetime
        std::string datetime = TimeUtils::get_current_time();

        // Build JSON transaction data
        nlohmann::json json_data;
        json_data["transaction_id"] = m_trans_id;
        json_data["transaction_datetime"] = datetime;
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
        
        if (res_trans_id == m_trans_id && status == "complete" && total_anomalies > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            update_snap_masks(res_trans_id);
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

void MainWindow::on_toolkit_test_clicked()
{
    // Open shared memory object
    auto patch_size = PATCH_SIZE;
    std::string shm_name = SHM_NAME_FRAMES;
    size_t buffer = 2448 * 2048 * MAX_FRAME_BATCH_SIZE;

    int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        std::cerr << "Failed to open shared memory object." << std::endl;
        return;
    }

    // Resize shared memory object to the initial data size
    if (ftruncate(shm_fd, buffer) == -1)
    {
        std::cerr << "Failed to resize shared memory object." << std::endl;
        ::close(shm_fd);
        return;
    }

    // Map shared memory into address space
    void *shm_ptr = mmap(0, buffer, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        std::cerr << "Failed to map shared memory." << std::endl;
        ::close(shm_fd);
        return;
    }

    auto frame_width = m_image_pixbuf_toolkit->get_width();
    auto frame_height = m_image_pixbuf_toolkit->get_height();

    // Create a new buffer to hold a single channel (grayscale)
    std::vector<uint8_t> single_channel_data(frame_width * frame_height);

    // Get the pointer to the image data (raw pixel data)
    uint8_t* pData = reinterpret_cast<uint8_t*>(m_image_pixbuf_toolkit->get_pixels());

    // Iterate over each pixel and extract the red channel (index 0 = Red, 1 = Green, 2 = Blue)
    for (int i = 0; i < frame_width * frame_height; ++i) {
        uint8_t gray = pData[i * RGB_CHANNELS];  // Take data from the Red channel
        single_channel_data[i] = gray;           // Set grayscale pixel based on Red channel
    }

    // Copy the frame data into the calculated memory location
    auto frame_size = frame_width * frame_height;
    void* frame_ptr = static_cast<uint8_t*>(shm_ptr);
    std::memcpy(frame_ptr, single_channel_data.data(), frame_size);

    // Save the frame metadata
    std::vector<FrameOffsetInfo> frame_offsets;
    frame_offsets.push_back({ 0, frame_size, "N/A" });
    
    if (!frame_offsets.empty())
    {
        // Send the command to the ws server
        m_trans_id = generate_transaction_id();

        // Get confidence threshold and pixel threshold from settings file
        double confidence_threshold = 0.5;
        double pixel_threshold = 0.03;

        auto detection_settings = SettingsService::get_settings("detection");
        if (!detection_settings.empty())
        {
            confidence_threshold = detection_settings["detection_settings"];
            pixel_threshold = detection_settings["detection_settings"];
        }

        // Get current Datetime
        std::string datetime = TimeUtils::get_current_time();

        // Build JSON transaction data
        nlohmann::json json_data;
        json_data["transaction_id"] = m_trans_id;
        json_data["transaction_datetime"] = datetime;
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
        
        if (res_trans_id == m_trans_id && status == "complete" && total_anomalies > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            update_snap_masks(res_trans_id);
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
}

void MainWindow::update_snap_masks(std::string trans_id)
{
    std::filesystem::path trans_json = AppPaths::Project_Detection_Results_Path(TEMP_PROJECT_NAME) / trans_id / "transaction_data.json";
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
    int frame_height = json_data["frame_height"].get<int>();

    // Create a transparent mask pixbuf of the same size as the image
    m_mask_pixbuf_toolkit = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, frame_height);
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

    // Iterate through the pixels and modify the RGB channel
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

    if (n_channels != 4)
        return;

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
    auto now = std::chrono::system_clock::now();
    // Add a new session with the current time as the start and a placeholder for the end time
    m_session_times.emplace_back(now, std::chrono::system_clock::time_point::max());

    // Add or update 'session_times' in json poject file
    auto project_file_path = AppPaths::Project_Path(m_curr_project_name) / (m_curr_project_name + ".dscanproj");
    auto project_json_optional = FileUtils::get_json(project_file_path.string());
    if (project_json_optional.has_value())
    {
        auto &project_json = project_json_optional.value();
        auto session_times_json = nlohmann::json::array();
        for (const auto& session : m_session_times) {
            auto start_time = TimeUtils::get_formatted_time(session.first);
            auto end_time = session.second == std::chrono::system_clock::time_point::max()
                            ? "max" // Represent "max" as a string
                            : TimeUtils::get_formatted_time(session.second);

            session_times_json.push_back({
                {"start_time", start_time},
                {"end_time", end_time}
            });
        }

        if (!project_json.contains("session_times")) 
        {
            project_json["session_times"] = nlohmann::json::object();
        }
        project_json["session_times"] = session_times_json;

        // Write JSON to the file
        std::ofstream ofs(project_file_path);
        if (!ofs.is_open())
        {
            std::cerr << "Failed to open file: " << project_file_path << std::endl;
        }
        else
        {
            ofs << project_json.dump(4); // Pretty-print with 4 spaces
            ofs.close();
        }
    }

    if (m_rt_monitoring_detection_start_time_lbl)
    {
        // Get the current time
        auto now = TimeUtils::get_current_time();

        // Show start time
        m_rt_monitoring_detection_start_time_lbl->set_text(now);
    }

    if (m_rt_monitoring_num_anomalies_lbl)
    {
        m_rt_monitoring_num_anomalies_lbl->set_text("0");
    }

    // Ensure there's no existing processing thread running
    if (m_processing_thread.joinable()) 
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }
    m_frame_queue.clear();

    // Retrieve camera settings (frame width and height) from settings.ini file
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

    if (serial_numbers.empty())
    {
        // Create the message dialog with the specified parent window, message text, and button options
        Gtk::MessageDialog dialog(*this, 
                                "Camera(s) has not been configured.", 
                                false,
                                Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK,
                                true);
        dialog.set_secondary_text("Please go to the Settings page to configure camera(s) before running detection.");
        
        // Run the dialog and wait for the user to press the OK button
        dialog.run();
        return;
    }

    auto cam_settings = SettingsService::get_settings(serial_numbers[0]);
    int frame_width = 0, frame_height = 0;
    
    if (!cam_settings.empty() && cam_settings.contains("width"))
    {
        frame_width = cam_settings["width"];
    }

    if (!cam_settings.empty() && cam_settings.contains("height"))
    {
        frame_height = cam_settings["height"];
    }

    if (frame_width < PATCH_SIZE || frame_height < PATCH_SIZE)
    {
        // Create the message dialog with the specified parent window, message text, and button options
        Gtk::MessageDialog dialog(*this, 
                                "Frame width or height is smaller than the specified patch size.", 
                                false,
                                Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK,
                                true);
        dialog.set_secondary_text("Please go to the Settings page to update camera(s) settings before running detection.");
        
        // Run the dialog and wait for the user to press the OK button
        dialog.run();
        return;
    }

    // Gdk::Pixbuf does not directly support a single-channel format, 
    // so still create an RGB pixbuf and replicate the grayscale values across the three color channels.
    if (!m_image_pixbuf_rt_monitoring)
    {
        m_image_pixbuf_rt_monitoring = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, frame_width, frame_height * 2);
    }
    if (!m_mask_pixbuf_rt_monitoring)
    {
        m_mask_pixbuf_rt_monitoring = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, frame_height * 2);
    }

    size_t total_frame_rgb_size = 0;
    for (int i = 0; i < MAX_FRAME_BATCH_SIZE; ++i)
    {
        total_frame_rgb_size += frame_width * frame_height * RGB_CHANNELS;
    }

    if (m_frame_rgb_data_buffer.size() < total_frame_rgb_size)
    {
        m_frame_rgb_data_buffer.resize(total_frame_rgb_size);
    }

    size_t total_patch_rgba_size = 0;
    for (int i = 0; i < MAX_PATCHES_PER_BATCH; ++i)
    {
        total_patch_rgba_size += PATCH_SIZE * PATCH_SIZE * RGBA_CHANNELS;
    }

    if (m_patch_rgba_data_buffer.size() < total_patch_rgba_size)
    {
        m_patch_rgba_data_buffer.resize(total_patch_rgba_size);
    }

    size_t shm_frames_buffer = frame_width * frame_height * (MAX_FRAME_BATCH_SIZE + 1); // add extra one frame for safety

    // Start the frame processing thread
    m_processing_thread = std::thread([this, shm_frames_buffer]()
    {
        // Open shared memory object
        std::cout << "Open shared memory object" << std::endl;
        int shm_fd = shm_open(SHM_NAME_FRAMES.c_str(), O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1)
        {
            std::cerr << "Failed to open shared memory object." << std::endl;
            return;
        }

        // Resize shared memory object to the initial data size
        std::cout << "Resize shared memory object" << std::endl;
        if (ftruncate(shm_fd, shm_frames_buffer) == -1)
        {
            std::cerr << "Failed to resize shared memory object." << std::endl;
            ::close(shm_fd);
            return;
        }

        // Map shared memory into address space
        std::cout << "Map shared memory" << std::endl;
        void *shm_ptr = mmap(0, shm_frames_buffer, PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED)
        {
            std::cerr << "Failed to map shared memory." << std::endl;
            ::close(shm_fd);
            return;
        }

        // Open predictions shared memory
        int shm_fd_pred = shm_open(SHM_NAME_PREDICTIONS.c_str(), O_RDONLY, 0666);
        if (shm_fd_pred == -1)
        {
            std::cerr << "Failed to open shared memory." << std::endl;
            return;
        }

        // Map the entire shared memory region
        size_t shm_size_pred = PATCH_SIZE * PATCH_SIZE * MAX_PATCHES_PER_BATCH + sysconf(_SC_PAGESIZE); // Adjust for metadata
        void* shm_ptr_pred = mmap(nullptr, shm_size_pred, PROT_READ, MAP_SHARED, shm_fd_pred, 0);
        if (shm_ptr_pred == MAP_FAILED) {
            std::cerr << "Failed to map shared memory." << std::endl;
            return;
        }

        FrameData frame_data(nullptr, nullptr, ""); // Initialize FrameData with null pointers
        std::vector<FrameOffsetInfo> frame_offsets;
        std::vector<PatchPosition> patch_positions;
        m_session_anomaly_count = 0;
        
        while (m_is_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Prevent CPU overuse
            frame_offsets.clear();
            size_t offset = 0;
            int num_frames_dequeued = 0;
            int frame_width = 0;
            int frame_height = 0;
            uint8_t* frame_rgb_data_ptr = m_frame_rgb_data_buffer.data(); // Reset to the beginning of the buffer

            // sort by serial number of each frame
            std::vector<FrameData> sorted_frames;
            while (!m_frame_queue.isEmpty() && num_frames_dequeued < MAX_FRAME_BATCH_SIZE)
            {
                if (m_frame_queue.dequeue(frame_data))
                {
                    sorted_frames.push_back(frame_data);
                    num_frames_dequeued++;
                }
            }
            std::sort(sorted_frames.begin(), sorted_frames.end(), [](const FrameData& a, const FrameData& b)
            {
                return a.serial_number < b.serial_number;
            });

            // Copy the frame data into the shared memory
            for (auto frame_data : sorted_frames)
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

                // Convert Mono8 to RGB directly into the allocated RGB buffer
                uint8_t* current_rgb_frame = frame_rgb_data_ptr;
                for (size_t i = 0; i < frame_width * frame_height; ++i)
                {
                    uint8_t gray = frame_data.pData[i];
                    current_rgb_frame[i * RGB_CHANNELS + 0] = gray; // Red channel
                    current_rgb_frame[i * RGB_CHANNELS + 1] = gray; // Green channel
                    current_rgb_frame[i * RGB_CHANNELS + 2] = gray; // Blue channel
                }
                
                // Update the pointer to the next RGB frame
                frame_rgb_data_ptr += frame_width * frame_height * RGB_CHANNELS;
            }

            if (frame_offsets.empty())
            {
                continue;
            }

            // Ensure thread safety
            if (!m_images_dispatcher_connection.connected())
            {
                m_images_dispatcher_connection = m_main_images_dispatcher.connect([this, &frame_width, &frame_height, &num_frames_dequeued]()
                {
                    m_images_dispatcher_running = true;

                    int total_height = frame_height * num_frames_dequeued;
                    int current_y = 0;
                    int row_stride = frame_width * RGB_CHANNELS;
                    uint8_t* data_ptr = m_frame_rgb_data_buffer.data();
                    
                    for (int i = 0; i < num_frames_dequeued; ++i)
                    {
                        // Get a pointer to the RGB data for the current frame in the buffer.
                        uint8_t* frame_data_ptr = data_ptr + (i * row_stride * frame_height);

                        auto image_pixbuf = Gdk::Pixbuf::create_from_data(
                            frame_data_ptr,     // Pointer to the current frame's RGB data
                            Gdk::COLORSPACE_RGB,// Gdk::Pixbuf expects RGB data
                            false,              // No alpha channel
                            8,                  // 8 bits per channel
                            frame_width,
                            frame_height,
                            row_stride          // Row stride
                        );

                        if (image_pixbuf)
                        {
                            image_pixbuf->copy_area(
                                0,
                                0,
                                frame_width,
                                frame_height,
                                this->m_image_pixbuf_rt_monitoring,
                                0,
                                current_y
                            );
                            current_y += frame_height;
                        }
                    }

                    // Get the current size of the drawing area
                    int current_width = 0, current_height = 0;
                    this->m_rt_monitoring_drawing_area->get_size_request(current_width, current_height);

                    // Check if resizing is necessary
                    if (current_width != frame_width || current_height != total_height)
                    {
                        this->m_rt_monitoring_drawing_area->set_size_request(frame_width, total_height);
                    }
                    
                    // Redraw the drawing area
                    this->m_rt_monitoring_drawing_area->queue_draw();

                    m_images_dispatcher_running = false;
                });
            }

            m_main_images_dispatcher.emit();

            // Generate a transaction ID
            m_trans_id = generate_transaction_id();

            // Get confidence threshold and pixel threshold from settings file
            double confidence_threshold = 0.5;
            double pixel_threshold = 0.1;

            auto detection_settings = SettingsService::get_settings("detection");
            if (!detection_settings.empty())
            {
                confidence_threshold = detection_settings["confidence_threshold"];
                pixel_threshold = detection_settings["pixel_threshold"];
            }

            // Get current Datetime
            std::string datetime = TimeUtils::get_current_time();

            // Send the command to the ws server
            nlohmann::json json_data;
            json_data["project_name"] = m_curr_project_name;
            json_data["transaction_id"] = m_trans_id;
            json_data["transaction_datetime"] = datetime;
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
            
            if (!m_is_running)
            {
                std::cout << "Break out the loop while waiting for a WS response" << std::endl;
                break;
            }

            // Parse the JSON response
            nlohmann::json response_json = nlohmann::json::parse(m_ws_response);

            // Extract values from the JSON object
            std::string res_trans_id = response_json["transaction_id"];
            if (res_trans_id != m_trans_id)
            {
                std::cout << "Transaction ID mismatch" << std::endl;
                continue;
            }

            std::string status = response_json["status"];
            int total_anomalies = response_json["total_anomalies"];
            int patch_size = response_json["patch_size"];

            if (status == "complete")
            {
                if (total_anomalies > 0)
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

                    // Read metadata (aligned offsets)
                    char* metadata_start = static_cast<char*>(shm_ptr_pred) + (shm_size_pred - (total_anomalies * sizeof(uint32_t)));
                    
                    // Check alignment
                    if (reinterpret_cast<uintptr_t>(metadata_start) % alignof(uint32_t) != 0) {
                        std::cerr << "Metadata is not properly aligned for uint32_t!" << std::endl;
                        return;
                    }

                    // Cast to uint32_t* safely
                    uint32_t* metadata_ptr = reinterpret_cast<uint32_t*>(metadata_start);
                    std::vector<uint32_t> offsets;

                    // Read metadata into offsets
                    for (int i = 0; i < total_anomalies; i++) {
                        offsets.push_back(metadata_ptr[i]);
                    }
                    // Print the offsets
                    // std::cout << "Offsets: ";
                    // for (const auto& offset : offsets) {
                    //     std::cout << offset << " ";
                    // }
                    // std::cout << std::endl;

                    // Load and position each prediction
                    auto predictions = response_json["predictions"];

                    int predictions_per_row = frame_width / patch_size;
                    if (frame_width % patch_size != 0)
                    {
                        predictions_per_row++; // Allow for an additional prediction if there's remaining space
                    }

                    int i = 0;
                    uint8_t* patch_rgba_data_ptr = m_patch_rgba_data_buffer.data(); // Reset to the beginning of the buffer

                    patch_positions.clear();

                    for (uint32_t offset : offsets)
                    {
                        uint8_t* mono8_data = static_cast<uint8_t*>(shm_ptr_pred) + offset;

                        // Convert Mono8 to RGBA for Gdk::Pixbuf
                        uint8_t* current_rgba_patch = patch_rgba_data_ptr;
                        for (int i = 0; i < patch_size * patch_size; ++i)
                        {   
                            // For every pixel
                            unsigned char gray = mono8_data[i];
                            current_rgba_patch[i * RGBA_CHANNELS + 0] = gray; // Red channel
                            current_rgba_patch[i * RGBA_CHANNELS + 1] = gray; // Green channel
                            current_rgba_patch[i * RGBA_CHANNELS + 2] = gray; // Blue channel
                            current_rgba_patch[i * RGBA_CHANNELS + 3] = 255;  // Alpha channel (fully opaque)
                        }

                        // Update the pointer to the next patch
                        patch_rgba_data_ptr += patch_size * patch_size * RGBA_CHANNELS;

                        // Calculate row and column based on the index
                        int prediction_id = predictions[i++].get<int>();
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

                        patch_positions.emplace_back(position_x, position_y);
                    }
                }

                // Ensure thread safety
                if (!m_masks_dispatcher_connection.connected())
                {
                    m_masks_dispatcher_connection = m_main_masks_dispatcher.connect([this, &patch_positions, &patch_size, &total_anomalies]()
                    {
                        m_masks_dispatcher_running = true;
                    
                        m_mask_pixbuf_rt_monitoring->fill(0x00000000);

                        if (total_anomalies > 0)
                        {
                            // Update # of detected anomalies label
                            if (m_rt_monitoring_num_anomalies_lbl)
                            {
                                m_session_anomaly_count += total_anomalies;
                                m_rt_monitoring_num_anomalies_lbl->set_text(std::to_string(m_session_anomaly_count));
                            }

                            uint8_t* data_ptr = m_patch_rgba_data_buffer.data();
                            int row_stride = patch_size * RGBA_CHANNELS;                     
                            int i = 0;

                            for (auto patch_pos : patch_positions)
                            {
                                uint8_t* patch_data_ptr = data_ptr + (i * row_stride * patch_size);
                                auto position_x = patch_pos.position_x;
                                auto position_y = patch_pos.position_y;
                                // std::cout << "position_x: " << position_x << " position_y: " << position_y << std::endl;

                                // Create a Pixbuf from the aligned offset
                                auto prediction_pixbuf = Gdk::Pixbuf::create_from_data(
                                    patch_data_ptr,
                                    Gdk::COLORSPACE_RGB,
                                    true, // Has alpha
                                    8,    // 8 bits per channel
                                    patch_size,
                                    patch_size,
                                    row_stride
                                );

                                if (!prediction_pixbuf)
                                {
                                    std::cerr << "Failed to load prediction image" << std::endl;
                                    continue;
                                }
                                else
                                {
                                    std::cout << "prediction_pixbuf created" << std::endl;
                                }

                                // Update mask pixel buf
                                update_mask_color(prediction_pixbuf);
                                update_mask_alpha(prediction_pixbuf, this->m_mask_alpha * 255);

                                // Copy the prediction image into m_mask_pixbuf_rt_monitoring at the specified position
                                prediction_pixbuf->Gdk::Pixbuf::copy_area(
                                    0,
                                    0,
                                    prediction_pixbuf->get_width(),
                                    prediction_pixbuf->get_height(),
                                    this->m_mask_pixbuf_rt_monitoring,
                                    position_x,
                                    position_y
                                );
                                prediction_pixbuf.reset();

                                i++;
                            }
                        }
                        
                        // Redraw the drawing area
                        this->m_rt_monitoring_drawing_area->queue_draw();

                        m_masks_dispatcher_running = false;
                    });
                }
            }

            m_main_masks_dispatcher.emit();
        }

        // Clean up
        while (m_images_dispatcher_running || m_masks_dispatcher_running) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        frame_offsets.clear();
        patch_positions.clear();

        std::cout << "Clean up shared memory object" << std::endl;
        if (munmap(shm_ptr, shm_frames_buffer) == -1) // Unmap the shared memory
        {
            std::cerr << "Failed to unmap shared memory." << std::endl;
        }
        ::close(shm_fd);

        std::cout << "Clean up predictions shared memory object" << std::endl;
        if (munmap(shm_ptr_pred, shm_size_pred) == -1)
        {
            std::cerr << "Failed to unmap shared memory." << std::endl;
        }
        ::close(shm_fd_pred);

        m_image_pixbuf_rt_monitoring->fill(0x000000); // Fill with black
        m_mask_pixbuf_rt_monitoring->fill(0x00000000); // Fill with black
        // Redraw the drawing area
        this->m_rt_monitoring_drawing_area->queue_draw();
    });
}

void MainWindow::stop_detection()
{
    auto now = std::chrono::system_clock::now();
    if (!m_session_times.empty() &&
        m_session_times.back().second == std::chrono::system_clock::time_point::max())
    {
        // Update the end time of the last session
        m_session_times.back().second = now;

        // Update 'session_times' in json poject file
        auto project_file_path = AppPaths::Project_Path(m_curr_project_name) / (m_curr_project_name + ".dscanproj");
        auto project_json_optional = FileUtils::get_json(project_file_path.string());
        if (project_json_optional.has_value())
        {
            auto &project_json = project_json_optional.value();
            auto session_times_json = nlohmann::json::array();
            for (const auto& session : m_session_times) {
                auto start_time = TimeUtils::get_formatted_time(session.first);
                auto end_time = session.second == std::chrono::system_clock::time_point::max()
                                ? "max" // Represent "max" as a string
                                : TimeUtils::get_formatted_time(session.second);

                session_times_json.push_back({
                    {"start_time", start_time},
                    {"end_time", end_time}
                });
            }

            if (project_json.contains("session_times")) 
            {
                project_json["session_times"] = session_times_json;
            }

            // Write JSON to the file
            std::ofstream ofs(project_file_path);
            if (!ofs.is_open())
            {
                std::cerr << "Failed to open file: " << project_file_path << std::endl;
            }
            else
            {
                ofs << project_json.dump(4); // Pretty-print with 4 spaces
                ofs.close();
            }
        }
    }
    else
    {
        // Handle cases where no active session exists
        std::cerr << "No active session to stop." << std::endl;
    }

    // Get the current time
    auto now_formatted = TimeUtils::get_current_time();

    // Save stop time to settings file
    nlohmann::json new_setting;
    new_setting["last_session_end_time"] = now_formatted;
    SettingsService::add_or_update_settings("detection", new_setting);

    if (m_processing_thread.joinable())
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }

    if (m_rt_monitoring_detection_start_time_lbl)
    {
        m_rt_monitoring_detection_start_time_lbl->set_text("");
    }

    if (m_rt_monitoring_num_anomalies_lbl)
    {
        m_rt_monitoring_num_anomalies_lbl->set_text("");
    }

    if (m_images_dispatcher_connection.connected())
    {
        m_images_dispatcher_connection.disconnect();
    }

    if (m_masks_dispatcher_connection.connected())
    {
        m_masks_dispatcher_connection.disconnect();
    }

    // RetentionManager retention_manager;
    // retention_manager.enforce_daily_limit();
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
        auto timeout_duration = std::chrono::milliseconds(2500);
        if (m_ws_response_cv.wait_for(lock, timeout_duration, [this]{ return m_ws_response_ready; }))
        {
            // Response received within timeout
            std::cout << "receiving a ws response: " << m_ws_response << std::endl;
            m_logger->log("receiving a ws response: " + m_ws_response);

            // Reset the condition for future use if needed
            m_ws_response_ready = false;
        }
        else
        {
            // Timeout occurred
            std::cerr << "Timeout waiting for ws response" << std::endl;
            m_logger->log("Timeout waiting for ws response", Logger::ERROR);

            // Create a JSON object
            nlohmann::json json_obj;
            
            // Add the key-value pair
            json_obj["transaction_id"] = "N/A";

            // Convert to a JSON string
            m_ws_response = json_obj.dump();
        }
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

        update_mask_alpha(m_mask_pixbuf_explorer, m_mask_alpha * 255);
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