#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip> // For std::put_time
#include <gtkmm.h>
#include "report_window.h"
#include "file_utils.h"
#include "time_utils.h"
#include "app_paths.h"
#include "settings_service.h"

ReportWindow::ReportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &ReportWindow::on_window_shown));
    
    m_refGlade->get_widget("report_time_range_selector_cbox", m_report_time_range_selector_cbox);
    if (m_report_time_range_selector_cbox)
    {
        m_report_time_range_selector_cbox->signal_changed().connect(sigc::mem_fun(*this, &ReportWindow::on_report_time_range_selector_changed));
    }

    m_refGlade->get_widget("report_progress_bar", m_report_progress_bar);

    m_refGlade->get_widget("report_start_time_lbl", m_report_start_time_lbl);

    m_refGlade->get_widget("report_end_time_lbl", m_report_end_time_lbl);

    m_refGlade->get_widget("report_masking_switch", m_report_masking_switch);
    if (m_report_masking_switch)
    {
        // Get the PropertyProxy for the active property of the switch
        Glib::PropertyProxy<bool> active_property = m_report_masking_switch->property_active();

        // Connect to the signal_changed() of the PropertyProxy
        active_property.signal_changed().connect(sigc::mem_fun(*this, &ReportWindow::on_enable_masking_changed));
    }

    m_refGlade->get_widget("report_transactions_listbox", m_report_transactions_listbox);
    if (m_report_transactions_listbox)
    {
        m_report_transactions_listbox->signal_row_selected().connect(sigc::mem_fun(*this, &ReportWindow::on_transaction_selected));
    }

    m_refGlade->get_widget("report_display_area", m_report_image_display_area);
    if (m_report_image_display_area)
    {
        m_report_image_display_area->signal_draw().connect(sigc::mem_fun(*this, &ReportWindow::on_report_display_area_draw));

        // Connect mouse scroll event
        m_report_image_display_area->add_events(Gdk::SCROLL_MASK);
        m_report_image_display_area->signal_scroll_event().connect(sigc::mem_fun(*this, &ReportWindow::on_report_display_area_scroll_event));

        // // Connect mouse press and motion events
        m_report_image_display_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_report_image_display_area->signal_button_press_event().connect(sigc::mem_fun(*this, &ReportWindow::on_report_display_area_btn_press_event));
        m_report_image_display_area->signal_button_release_event().connect(sigc::mem_fun(*this, &ReportWindow::on_report_display_area_btn_release_event));
        m_report_image_display_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &ReportWindow::on_report_display_area_motion_notify_event));
    }

    m_refGlade->get_widget("report_timeline_drawing_area", m_report_timeline_drawing_area);
    if (m_report_timeline_drawing_area)
    {
        m_report_timeline_drawing_area->signal_draw().connect(sigc::mem_fun(*this, &ReportWindow::on_report_timeline_draw));
    }
    
    m_refGlade->get_widget("detection_result_datetime_lbl", m_detection_result_datetime_lbl);

    m_refGlade->get_widget("detection_result_path_lbl", m_detection_result_path_lbl);

    m_refGlade->get_widget("moving_speed_entry", m_moving_speed_entry);

    m_refGlade->get_widget("save_report_btn", m_save_report_btn);
    if (m_save_report_btn)
    {
        m_save_report_btn->signal_clicked().connect(sigc::mem_fun(*this, &ReportWindow::on_save_report_clicked));
    }
}

ReportWindow *ReportWindow::create(const std::string &gladeFilePath)
{
    // Load the Glade file
    auto refBuilder = Gtk::Builder::create_from_file(gladeFilePath);

    // Get the window object from the Glade file
    ReportWindow *window = nullptr;
    refBuilder->get_widget_derived("report_window", window);

    return window;
}

void ReportWindow::on_window_shown()
{
    this->set_title("Eagle Eye - Create Reports");
}

bool ReportWindow::on_key_press_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrl_pressed = true;
    }
    return Gtk::Window::on_key_press_event(key_event);
}

bool ReportWindow::on_key_release_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrl_pressed = false;
    }
    return Gtk::Window::on_key_release_event(key_event);
}

void ReportWindow::on_enable_masking_changed()
{
    m_show_mask_detection_result = m_report_masking_switch->get_active();
    Gtk::ListBoxRow* selected_row = m_report_transactions_listbox->get_selected_row();
    if (selected_row)
    {
        on_transaction_selected(selected_row);
    }
}

bool ReportWindow::on_report_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x, m_offset_y);   // Apply panning offset
    cr->scale(m_zoom_factor, m_zoom_factor); // Apply zoom

    // Draw the images
    if (m_image_pixbuf_detection_result)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_image_pixbuf_detection_result, 0, 0);
        cr->paint();
    }

    // Draw the masks
    if (m_show_mask_detection_result && m_mask_pixbuf_detection_result)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_mask_pixbuf_detection_result, 0, 0);
        cr->paint();
    }

    return true;
}

bool ReportWindow::on_report_display_area_scroll_event(GdkEventScroll *scroll_event)
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
            m_zoom_factor += zoom_step;
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_zoom_factor = std::max(zoom_step, m_zoom_factor - zoom_step);
        }
    }

    // Trigger a redraw of the drawing area
    m_report_image_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

bool ReportWindow::on_report_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging = true;
        m_drag_start_x = button_event->x;
        m_drag_start_y = button_event->y;
    }

    // Return true to indicate that the event has been handled
    return true;
}

bool ReportWindow::on_report_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging = false;
    }
    
    // Return true to indicate that the event has been handled
    return true;
}

bool ReportWindow::on_report_display_area_motion_notify_event(GdkEventMotion *motion_event)
{
    if (m_is_dragging)
    {
        // Calculate the distance moved
        double deltaX = motion_event->x - m_drag_start_x;
        double deltaY = motion_event->y - m_drag_start_y;

        // Update the panning offset
        m_offset_x += deltaX;
        m_offset_y += deltaY;

        // Update the start position for the next motion event
        m_drag_start_x = motion_event->x;
        m_drag_start_y = motion_event->y;
    }

    // Trigger a redraw of the drawing area
    m_report_image_display_area->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

void ReportWindow::load_detection_result(std::string &detection_result_folder)
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
    m_image_pixbuf_detection_result = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    // m_image_pixbuf_detection_result->fill(0xffffffbe); // For testing
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

    // Preserve the original frame pixbuf
    auto frame_pixbuf_original = m_image_pixbuf_detection_result;
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
        m_report_image_display_area->set_size_request(frame_width, total_height);
        m_report_image_display_area->queue_draw();
    }
}

bool ReportWindow::on_report_timeline_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Get the DrawingArea dimensions
    int width = m_report_timeline_drawing_area->get_allocated_width();
    int height = m_report_timeline_drawing_area->get_allocated_height();

    // Draw the timeline (horizontal line)
    cr->set_line_width(height);
    cr->set_source_rgb(0, 0, 0); // Black
    cr->move_to(0, height / 2);
    cr->line_to(width, height / 2);
    cr->stroke();

    auto start_time_str = m_report_start_time_lbl->get_text();
    auto end_time_str = m_report_end_time_lbl->get_text();

    if (start_time_str == "N/A" || end_time_str == "N/A")
    {
        return true;
    }

    auto start_tp = TimeUtils::parse_time(start_time_str);
    auto end_tp = TimeUtils::parse_time(end_time_str);
    auto session_duration = end_tp - start_tp;

    // Draw detection results
    for (const auto &result_folder : m_detection_results_in_report)
    {
        auto creation_time = FileUtils::get_creation_time(result_folder.string());
        if (creation_time.has_value())
        {
            auto x = (*creation_time - start_tp) * width / session_duration;
            auto creation_time_time_t = std::chrono::system_clock::to_time_t(*creation_time);

            // Draw the vertical line
            cr->set_line_width(2.0);
            cr->set_source_rgb(1.0, 0.5, 0.0); // Amber
            cr->move_to(x, 0);
            cr->line_to(x, height);
            cr->stroke();
        }
        else
        {
            std::cerr << "Error: Creation time is not available for this folder." << std::endl;
        }
    }

    // Highlight selected result
    auto creation_time = FileUtils::get_creation_time(m_selected_detection_result_in_report);
    if (creation_time.has_value())
    {
        // Draw a triangle at the top of the selected event line
        auto x = (*creation_time - start_tp) * width / session_duration;
        const double triangle_size = 10.0; // Size of the triangle
        cr->set_source_rgb(1.0, 0.5, 0.0); // Amber
        cr->move_to(x, 15);           // Top point of the triangle
        cr->line_to(x - triangle_size, 0); // Bottom-left point
        cr->line_to(x + triangle_size, 0); // Bottom-right point
        cr->close_path();
        cr->fill();

        // // Draw event position text at the bottom
        // auto creation_time_time_t = std::chrono::system_clock::to_time_t(*creation_time);
        // cr->set_source_rgb(0.0, 0.0, 0.0); // Black
        // std::ostringstream oss;
        // oss << ctime(&creation_time_time_t);
        // cr->move_to(x - 30, height - 5); // Slightly offset for better alignment
        // cr->show_text(oss.str());
        // cr->stroke();
    }
    else
    {
        std::cerr << "Error: Creation time is not available for this folder." << std::endl;
    }

    return true;
}

void ReportWindow::on_report_time_range_selector_changed()
{
    auto selected_time_range = m_report_time_range_selector_cbox->get_active_text();
    std::string start_time = "N/A";
    std::string end_time = TimeUtils::get_current_time();

    if (selected_time_range == "Last Session")
    {
        auto detection_settings = SettingsService::get_settings("detection");
        if (!detection_settings.empty())
        {
            if (detection_settings.contains("last_session_start_time"))
            {
                start_time = detection_settings["last_session_start_time"];
            }

            if (detection_settings.contains("last_session_end_time"))
            {
                end_time = detection_settings["last_session_end_time"];
            }
        }
    }
    else if (selected_time_range == "Past 15 Minutes")
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

    if (m_report_start_time_lbl)
    {
        m_report_start_time_lbl->set_text(start_time);
    }

    if (m_report_end_time_lbl)
    {
        m_report_end_time_lbl->set_text(end_time);
    }

    auto start_tp = TimeUtils::parse_time(start_time);
    auto end_tp = TimeUtils::parse_time(end_time);

    // Load transactions list
    m_detection_results_in_report = FileUtils::get_folders_by_time(AppPaths::Detection_Results_Path, start_tp, end_tp);

    // Clear the resutls before loading
    for (auto *child : m_report_transactions_listbox->get_children())
    {
        m_report_transactions_listbox->remove(*child);
    }

    // Populating the detection results list box with rows
    for (const auto &result_folder : m_detection_results_in_report)
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

    // Load timeline
    if (m_report_timeline_drawing_area)
    {
        m_report_timeline_drawing_area->queue_draw();
    }
}

void ReportWindow::on_transaction_selected(Gtk::ListBoxRow* row)
{
    if (row)
    {
        auto row_box = dynamic_cast<Gtk::Box*>(row->get_child());
        if (row_box)
        {
            m_selected_detection_result_in_report = row_box->get_tooltip_text();
            // std::cout << "Selected row: " << m_selected_detection_result_in_report << std::endl;
            load_detection_result(m_selected_detection_result_in_report);

            // Redraw timeline and indicator
            if (m_report_timeline_drawing_area)
            {
                m_report_timeline_drawing_area->queue_draw();
            }

            // Update properties panel
            if (m_detection_result_datetime_lbl)
            {
                std::string transaction_datetime = "N/A";
                std::filesystem::path trans_json_path = std::filesystem::path(m_selected_detection_result_in_report) / "transaction_data.json";
                if (std::filesystem::exists(trans_json_path))
                {
                    // Read the content of the JSON file
                    std::ifstream json_file(trans_json_path);
                    if (json_file.is_open())
                    {
                        // Parse the JSON content
                        nlohmann::json json_data;
                        try
                        {
                            json_file >> json_data;
                            json_file.close();

                            transaction_datetime = json_data["transaction_datetime"];
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                        }
                    }
                }

                m_detection_result_datetime_lbl->set_text(transaction_datetime);
            }

            if (m_detection_result_path_lbl)
            {
                m_detection_result_path_lbl->set_text(m_selected_detection_result_in_report);
            }
        }
    }
    else
    {
        std::cout << "No row selected!" << std::endl;
    }
}

void ReportWindow::on_save_report_clicked()
{
    // Create a FileChooserDialog in Save mode
    Gtk::FileChooserDialog dialog("Save File", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);

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

void ReportWindow::create_csv_file(const std::string &file_name)
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
    }

    // Write the headers
    csv_file << "Transaction ID,Date Time (yyyy-mm-dd hh:mm:ss.000),Interval (hh:mm:ss.000),Position (mm)\n";

    // Write the rows
    std::chrono::system_clock::time_point previous_time_point;
    bool is_first_transaction = true;

    for (const auto &result_folder : m_detection_results_in_report)
    {
        std::filesystem::path trans_json_path = result_folder / "transaction_data.json";
        if (std::filesystem::exists(trans_json_path))
        {
            // Read the content of the JSON file
            std::ifstream json_file(trans_json_path);
            if (json_file.is_open())
            {
                // Parse the JSON content
                nlohmann::json json_data;
                try
                {
                    json_file >> json_data;
                    json_file.close();

                    auto transaction_id = json_data["transaction_id"];
                    auto transaction_datetime = json_data["transaction_datetime"];
                    auto current_time_point = TimeUtils::parse_time(transaction_datetime);

                    // Calculate the duration since the last transaction
                    std::string duration_str = "00:00:00.000";
                    if (!is_first_transaction)
                    {
                        duration_str = TimeUtils::get_time_interval(current_time_point, previous_time_point);
                    }

                    // Calculate the position
                    double moving_speed = std::stod(m_moving_speed_entry->get_text());
                    double position = is_first_transaction ? 0.0 : std::chrono::duration_cast<std::chrono::duration<double>>(current_time_point - previous_time_point).count() * moving_speed;
                    
                    // Write the data row
                    csv_file << transaction_id << ','
                             << '=' << transaction_datetime << ','
                             << '=' << '"' << duration_str << '"' << ','
                             << position << '\n';

                    // Update previous_time_point and mark as not the first transaction
                    previous_time_point = current_time_point;
                    is_first_transaction = false;
                }
                catch (const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
        }
    }

    // Close the file
    csv_file.close();
    std::cout << "CSV file created successfully: " << file_name << std::endl;
}

void ReportWindow::update_mask_color(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf)
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

void ReportWindow::update_mask_alpha(Glib::RefPtr<Gdk::Pixbuf> mask_pixbuf, gint32 alpha)
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