#include "main_window.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_frame_queue(2),
      m_logger(logger)
{
    signal_show().connect(sigc::mem_fun(*this, &MainWindow::on_window_shown));
    signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_window_delete));

    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("main_window", root);
    root->set_title("Eagle Eye");

    m_builder->get_widget("capture_source_cbox", m_camera_combo_box);

    m_builder->get_widget("capture_rate_sb", m_capture_rate_sb);

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

    m_builder->get_widget("test_capture_source_cbox", m_camera_test_combo_box);

    m_builder->get_widget("test_btn", m_test_btn);
    if (m_test_btn)
    {
        m_test_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_test_clicked));
    }

    m_builder->get_widget("test_drawing_area", m_test_display_area);
    if (m_test_display_area)
    {
        m_test_display_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_test_display_area_draw));

        // Connect mouse scroll event
        m_test_display_area->add_events(Gdk::SCROLL_MASK);
        m_test_display_area->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_test_display_area_scroll_event));

        // Connect mouse press and motion events
        m_test_display_area->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_test_display_area->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_test_display_area_btn_press_event));
        m_test_display_area->signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_test_display_area_btn_release_event));
        m_test_display_area->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_test_display_area_motion_notify_event));
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_window_shown()
{
    discover_cameras();

    if (m_camList.nDeviceNum > 0)
    {
        for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
            if (pDeviceInfo == nullptr)
            {
                continue;
            }

            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
            {
                // Add the camera name to the combo box
                auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
                m_camera_combo_box->append(std::string((char *)serialNumber));
                m_camera_test_combo_box->append(std::string((char *)serialNumber));
            }
        }
        m_camera_combo_box->append("All Cameras");
    }
    else
    {
        std::cout << "No camera found." << std::endl;
        m_logger->log("No camera found.");
    }

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
    int nRet = 0;
    
    // Perform any necessary cleanup here
    for (void *device_handle : m_device_handles)
    {
        // Close the device
        nRet = MV_CC_CloseDevice(device_handle);
        if (nRet != MV_OK)
        {
            std::cerr << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
        }

        // Destory the device handle
        nRet = MV_CC_DestroyHandle(device_handle);
        if (nRet != MV_OK)
        {
            std::cerr << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
        }
    }

    // Disconnect from the WebSocket server
    m_ws_client.disconnect();

    // Returning false allows the window to close
    return false;
}

void MainWindow::discover_cameras()
{
    // enum device
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE, &m_camList);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_EnumDevices fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_EnumDevices: " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::on_start_clicked()
{
    m_is_capturing = true;
    m_start_btn->set_sensitive(!m_is_capturing);
    m_test_btn->set_sensitive(!m_is_capturing);

    double capture_interval_ms = 0.0;
    if (m_capture_rate_sb)
    {
        int capture_rate = m_capture_rate_sb->get_value();
        // Calculate the capture interval (in milliseconds) based on capture rate (FPS)
        capture_interval_ms = 1000.0 / static_cast<double>(capture_rate);
    }

    if (m_device_handles.empty())
    {
        if (m_camera_combo_box)
        {
            auto selectedCaptureDevice = m_camera_combo_box->get_active_text();
            if (selectedCaptureDevice == "All Cameras")
            {
                m_device_handles = get_all_device_handles();
            }
            else
            {
                // Create device handle
                auto deviceHandle = get_device_handle_by_serial_number(selectedCaptureDevice);
                if (deviceHandle == nullptr)
                {
                    std::cout << "get_device_handle_by_serial_number fail! deviceHandle is nullptr" << std::endl;
                    m_logger->log("Error on get_device_handle_by_serial_number: deviceHandle is nullptr.", Logger::ERROR);
                }
                m_device_handles.push_back(deviceHandle);
            }
        }

        // Preflight
        for (void *device_handle : m_device_handles)
        {
            auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            std::cout << "Preflight request to device: " << device_handle << " at " << currentTimeInMs << std::endl;
            m_logger->log("Preflight request to device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

            // Launch preflight asynchronously for each device
            std::async(std::launch::async, &MainWindow::preflight, this, device_handle);
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (void *device_handle : m_device_handles)
    {
        auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "Begin capture for device: " << device_handle << " at " << currentTimeInMs << std::endl;
        m_logger->log("Begin capture for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

        // Launch start_capture asynchronously for each device
        start_capture(device_handle, capture_interval_ms);
    }

    start_detection();
}

void MainWindow::on_stop_clicked()
{
    m_is_capturing = false;
    m_start_btn->set_sensitive(!m_is_capturing);
    m_test_btn->set_sensitive(!m_is_capturing);

    for (void *device_handle : m_device_handles)
    {
        m_logger->log("Stop capture for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));
        stop_capture(device_handle);
    }

    // Ensure there's no existing processing thread running
    if (m_processing_thread.joinable()) 
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }
}

void MainWindow::on_test_clicked()
{
    // Disconnect all cameras
    for (void *device_handle : m_device_handles)
    {
        // Close the device
        int nRet = MV_CC_CloseDevice(device_handle);
        if (nRet != MV_OK)
        {
            std::cerr << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
        }

        // Destory the device handle
        nRet = MV_CC_DestroyHandle(device_handle);
        if (nRet != MV_OK)
        {
            std::cerr << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
        }
    }
    m_device_handles.clear();

    auto selectedCaptureDevice = m_camera_test_combo_box->get_active_text();
    auto device_handle = get_device_handle_by_serial_number(selectedCaptureDevice);

    // Reset image pixel buffer and test frame
    if (m_ImagePixbuf)
    {
        m_ImagePixbuf.reset();
    }
    if (m_maskPixbuf)
    {
        m_maskPixbuf.reset();
    }

    // Reset zoom and pan when a new image is loaded
    m_zoom_factor = 1.0;
    m_offset_x = 0.0;
    m_offset_y = 0.0;
    
    // Connect to the device
    int nRet = MV_CC_OpenDevice(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_OpenDevice: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    m_logger->log("Connected to device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

    // Detect network optimal packet size(It only works for the GigE camera)
    int nPacketSize = MV_CC_GetOptimalPacketSize(device_handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValue(device_handle, "GevSCPSPacketSize", nPacketSize);
        if (nRet != MV_OK)
        {
            std::cout << "Set Packet Size fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(GevSCPSPacketSize): " + std::to_string(nRet), Logger::ERROR);
        }
    }
    else
    {
        std::cout << "Get Packet Size fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetOptimalPacketSize: " + std::to_string(nRet), Logger::ERROR);
    }
    m_logger->log("MV_CC_SetIntValue(GevSCPSPacketSize) for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

    // Enable trigger mode
    nRet = MV_CC_SetEnumValue(device_handle, "TriggerMode", 1);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerMode fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetTriggerMode: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Set trigger source
    nRet = MV_CC_SetEnumValue(device_handle, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerSource fail! Error code:" << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(TriggerSource): " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Start grabbing images
    nRet = MV_CC_StartGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StartGrabbing: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Capture one frame
    nRet = MV_CC_SetCommandValue(device_handle, "TriggerSoftware");
    if(MV_OK != nRet)
    {
        std::cout << "Error on TriggerSoftware: " << nRet << std::endl;
        m_logger->log("Error on TriggerSoftware: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Grab one frame from camera
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    nRet = MV_CC_GetIntValue(device_handle, "PayloadSize", &stParam);
    if (MV_OK != nRet)
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
    save_image(pData, stImageInfo, device_handle);

    // Load the frame from file    
    try
    {
        m_ImagePixbuf = Gdk::Pixbuf::create_from_file("/tmp/eagle_eye/test.jpeg");
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
    if (m_ImagePixbuf)
    {
        m_test_display_area->set_size_request(frame_width, frame_height);
        m_test_display_area->queue_draw();
    }

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

    for (int i = 0; i < num_frames; ++i)
    {
        // Save the frame metadata
        frame_offsets.push_back({ offset, frame_size, frame_width, patch_size });

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
        nlohmann::json json_data;
        auto trans_id = generate_transaction_id();
        json_data["transaction_id"] = trans_id;
        for (const auto& info : frame_offsets)
        {
            json_data["frames"].push_back({
                {"offset", info.offset},
                {"frame_size", info.frame_size},
                {"frame_width", info.frame_width},
                {"frame_height", info.frame_height}
            });
        }

        std::string frame_info_string = json_data.dump(); // Convert JSON to string
        send_ws_message(frame_info_string);

        // Parse the JSON response
        nlohmann::json response_json = nlohmann::json::parse(m_ws_response);

        // Extract values from the JSON object
        std::string res_trans_id = response_json["transaction_id"];
        std::string status = response_json["status"];

        if (res_trans_id == trans_id && status == "complete")
        {
            display_test_masks(res_trans_id);
        }
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

    // Close the device
    nRet = MV_CC_CloseDevice(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
    }

    // Destory the device handle
    nRet = MV_CC_DestroyHandle(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::display_test_masks(std::string trans_id)
{
    const char* home = std::getenv("HOME");
    std::filesystem::path trans_json = std::filesystem::path(home) / "eagle_eye" / "test_result" / trans_id / "transaction_data.json";
    if (!std::filesystem::exists(trans_json))
    {
        std::cerr << "File not exists: transaction_data.json" << std::endl;
        return;
    }

    // Read the content of the JSON file
    std::ifstream json_file(trans_json);
    if (!json_file.is_open()) {
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
    int frame_width = 0;
    int total_height = 0;

    // Access frames array
    for (const auto &frame : json_data["frames"])
    {
        frame_width = frame["frame_width"].get<int>();
        total_height += frame["frame_height"].get<int>();
    }

    // Create a transparent mask pixbuf of the same size as the image
    m_maskPixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, frame_width, total_height);
    // m_maskPixbuf->fill(0xffffffbe); // For testing
    m_maskPixbuf->fill(0x00000000); // Initialize the mask to be fully transparent black

    int predictions_per_row = frame_width / patch_size;
    if (frame_width % patch_size != 0)
    {
        predictions_per_row++; // Allow for an additional prediction if there's remaining space
    }

    // Access predictions array
    for (const auto &prediction : json_data["predictions"])
    {
        int prediction_id = prediction["prediction_id"].get<int>();
        std::string filename = prediction["filename"].get<std::string>();
        
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

        // Copy the prediction image into m_maskPixbuf at the specified position
        prediction_pixbuf->Gdk::Pixbuf::copy_area(
            0,
            0,
            prediction_pixbuf->get_width(),
            prediction_pixbuf->get_height(),
            m_maskPixbuf,
            position_x,
            position_y
        );
    }

    // Update mask pixel buf
    update_mask_color();
    update_mask_alpha(m_mask_alpha * 255);

    m_test_display_area->queue_draw();
}

void MainWindow::update_mask_color()
{
    const int AmberRed = 255;
    const int AmberGreen = 191;
    const int AmberBlue = 0;

    // Get pixbuf properties
    int mask_width = m_maskPixbuf->get_width();
    int mask_height = m_maskPixbuf->get_height();
    int mask_rowstride = m_maskPixbuf->get_rowstride();
    int mask_n_channels = m_maskPixbuf->get_n_channels();

    // Get pointer to the pixel data
    guchar *pixels = m_maskPixbuf->get_pixels();

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

void MainWindow::update_mask_alpha(gint32 alpha)
{
    if (!m_maskPixbuf)
        return;

    // Get pixbuf properties
    int width = m_maskPixbuf->get_width();
    int height = m_maskPixbuf->get_height();
    int rowstride = m_maskPixbuf->get_rowstride();
    int n_channels = m_maskPixbuf->get_n_channels();

    // Get pointer to the pixel data
    guchar *pixels = m_maskPixbuf->get_pixels();

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

bool MainWindow::on_test_display_area_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offset_x, m_offset_y);   // Apply panning offset
    cr->scale(m_zoom_factor, m_zoom_factor); // Apply zoom

    // Draw the image
    if (m_ImagePixbuf)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_ImagePixbuf, 0, 0);
        cr->paint();
    }

    if (m_maskPixbuf)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_maskPixbuf, 0, 0);
        cr->paint();
    }

    return true;
}

void *MainWindow::get_device_handle_by_serial_number(std::string sn)
{
    for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
            std::string serialNumberStr(reinterpret_cast<const char *>(serialNumber));
            if (serialNumberStr == sn)
            {
                void *deviceHandle;
                int nRet = MV_CC_CreateHandle(&deviceHandle, pDeviceInfo);
                if (nRet != MV_OK)
                {
                    std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
                    m_logger->log("Error on MV_CC_CreateHandle: " + std::to_string(nRet), Logger::ERROR);
                    return nullptr;
                }
                return deviceHandle;
            }
        }
    }

    return nullptr;
}

std::vector<void*> MainWindow::get_all_device_handles()
{
    std::vector<void*> deviceHandles;  // To store all device handles

    for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            void *deviceHandle;
            int nRet = MV_CC_CreateHandle(&deviceHandle, pDeviceInfo);
            if (nRet != MV_OK)
            {
                std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_CreateHandle: " + std::to_string(nRet), Logger::ERROR);
                continue;  // Skip this device if handle creation failed
            }

            deviceHandles.push_back(deviceHandle);  // Add handle to the list
        }
    }

    return deviceHandles;  // Return all device handles
}

void MainWindow::preflight(void *device_handle)
{
    // Connect to the device
    int nRet = MV_CC_OpenDevice(device_handle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_OpenDevice: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    m_logger->log("Connected to device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

    // Detect network optimal packet size(It only works for the GigE camera)
    int nPacketSize = MV_CC_GetOptimalPacketSize(device_handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValue(device_handle, "GevSCPSPacketSize", nPacketSize);
        if (nRet != MV_OK)
        {
            std::cout << "Set Packet Size fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(GevSCPSPacketSize): " + std::to_string(nRet), Logger::ERROR);
        }
    }
    else
    {
        std::cout << "Get Packet Size fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetOptimalPacketSize: " + std::to_string(nRet), Logger::ERROR);
    }
    m_logger->log("MV_CC_SetIntValue(GevSCPSPacketSize) for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

    // Enable trigger mode
    nRet = MV_CC_SetEnumValue(device_handle, "TriggerMode", 1);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerMode fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetTriggerMode: " + std::to_string(nRet), Logger::ERROR);
    }

    // Set trigger source
    nRet = MV_CC_SetEnumValue(device_handle, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerSource fail! Error code:" << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(TriggerSource): " + std::to_string(nRet), Logger::ERROR);
    }
    
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

        MainWindow *pThis = static_cast<MainWindow *>(pUser); // Cast pUser to MainWindow*
        pThis->m_frame_queue.enqueue(FrameData(pData, pFrameInfo));
    };

    m_logger->log("MV_CC_RegisterImageCallBackEx for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));
    nRet = MV_CC_RegisterImageCallBackEx(device_handle, image_capture_callback, this);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_RegisterImageCallBackEx fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_RegisterImageCallBackEx: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
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
        m_logger->log("Started MV_CC_StartGrabbing for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));

        std::string camera_handle = std::to_string(reinterpret_cast<uintptr_t>(device_handle));
        auto& capturing_thread = m_capturing_threads[camera_handle];

        // Ensure there's no existing capture thread running
        if (capturing_thread.joinable()) {
            capturing_thread.join();  // Wait for previous thread to finish
        }

        // Start the frame acquisition thread
        capturing_thread = std::thread([this, device_handle, capture_interval_ms]()
        {
            uint64_t lastCaptureTimestamp = 0;

            while (m_is_capturing)
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
    }
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
        FrameData frame_data(nullptr, nullptr); // Initialize FrameData with null pointers

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
        
        while (m_is_capturing)
        {
            // std::cout << "Entering processing loop" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
            
            size_t offset = 0;
            frame_offsets.clear();
            int frames_dequeued = 0;

            // Copy each frame into its respective memory offset
            while (!m_frame_queue.isEmpty() && frames_dequeued < 2)
            {
                if (m_frame_queue.dequeue(frame_data))
                {
                    auto frame_size = frame_data.pMetadata->nFrameLen;
                    auto frame_width = frame_data.pMetadata->nWidth;
                    auto frame_height = frame_data.pMetadata->nHeight;

                    // Save the frame metadata
                    frame_offsets.push_back({ offset, frame_size, frame_width, frame_height });

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

            // Send the command to the ws server
            nlohmann::json json_data;
            json_data["transaction_id"] = generate_transaction_id();
            for (const auto& info : frame_offsets)
            {
                json_data["frames"].push_back({
                    {"offset", info.offset},
                    {"frame_size", info.frame_size},
                    {"frame_width", info.frame_width},
                    {"frame_height", info.frame_height}
                });
            }
            std::string frame_info_string = json_data.dump(); // Convert JSON to string
            send_ws_message(frame_info_string);
            
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

void MainWindow::stop_capture(void *device_handle)
{
    std::string camera_handle = std::to_string(reinterpret_cast<uintptr_t>(device_handle));
    auto& capturing_thread = m_capturing_threads[camera_handle];

    // Ensure there's no existing capture thread running
    if (capturing_thread.joinable()) {
        capturing_thread.join();  // Wait for previous thread to finish
    }

    int nRet = MV_CC_StopGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StopGrabbing", Logger::ERROR);
    }
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

void MainWindow::save_image(unsigned char *pData, MV_FRAME_OUT_INFO_EX frameInfo, void *deviceHandle)
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
    sprintf(stSaveFileParam.pImagePath, "/tmp/eagle_eye/test.jpeg");
    
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

bool MainWindow::on_test_display_area_scroll_event(GdkEventScroll *scroll_event)
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

        update_mask_alpha(m_mask_alpha * 255);
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
    m_test_display_area->queue_draw();

    // Return true to indicate that the event has been handled
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

bool MainWindow::on_test_display_area_btn_press_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Start dragging
        m_is_dragging = true;
        m_drag_start_x = button_event->x;
        m_drag_start_y = button_event->y;
    }
    return true;
}

bool MainWindow::on_test_display_area_btn_release_event(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        // Stop dragging
        m_is_dragging = false;
    }
    return true;
}

bool MainWindow::on_test_display_area_motion_notify_event(GdkEventMotion *motion_event)
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
    m_test_display_area->queue_draw();

    return true;
}