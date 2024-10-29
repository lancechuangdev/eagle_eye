#include "main_window.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_frame_queue(20),
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

    m_builder->get_widget("load_model_fcb", m_load_model_fcb);

    m_builder->get_widget("prediction_result_fcb", m_prediction_result_fcb);

    m_builder->get_widget("confidence_threshold_sb", m_confidence_threshold_sb);

    m_builder->get_widget("pixel_threshold_sb", m_pixel_threshold_sb);

    m_builder->get_widget("py_env_entry", m_py_env_entry);

    m_builder->get_widget("model_patch_size_sb", m_patch_size_sb);

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
            }
        }
        m_camera_combo_box->append("All Cameras");
    }
    else
    {
        std::cout << "No camera found." << std::endl;
        m_logger->log("No camera found.");
    }

    // Write the Python script to the temp file
    m_py_script = "/tmp/eagle_eye/temp_unet_pred.py";
    if (!FileUtils::createSubdirectory("/tmp", "eagle_eye"))
    {
        std::cerr << "Failed to create tmp directory." << std::endl;
    }
    else
    {
        std::ofstream tempUnetPredPyFile(m_py_script);
        if (tempUnetPredPyFile.is_open())
        {
            tempUnetPredPyFile << unet_predict_py;
            tempUnetPredPyFile.close();
        }
        else
        {
            std::cerr << "Failed to open temp_unet_pred.py for writing" << std::endl;
            m_logger->log("Unable to open temp_unet_pred.py for writing", Logger::ERROR);
        }
    }
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

    // Delete the tmp script after execution
    std::remove(m_py_script.c_str());

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
        start_detection(m_py_script);
    }
}

void MainWindow::on_stop_clicked()
{
    m_isCapturing = false;

    for (void *device_handle : m_device_handles)
    {
        m_logger->log("Stop capture for device: " + std::to_string(reinterpret_cast<uintptr_t>(device_handle)));
        stop_capture(device_handle);
    }
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
        if (pFrameInfo)
        {
            std::cout << "GetOneFrame, nDevTimeStampHigh: " << pFrameInfo->nDevTimeStampHigh
                    << ", nDevTimeStampLow: " << pFrameInfo->nDevTimeStampLow
                    << ", nHostTimeStamp: " << pFrameInfo->nHostTimeStamp
                    << std::endl;
        }

        // Cast pUser to MainWindow*
        MainWindow *pThis = static_cast<MainWindow *>(pUser);

        std::cout << "m_frame_queue.enqueue started" << std::endl;  
        pThis->m_frame_queue.enqueue(FrameData(pData, pFrameInfo));
        std::cout << "m_frame_queue.enqueue ended" << std::endl;
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
        m_isCapturing = true;

        // Ensure there's no existing capture thread running
        if (m_capturing_thread.joinable()) 
        {
            m_capturing_thread.join();  // Wait for previous thread to finish
        }

        // Start the frame acquisition thread
        m_capturing_thread = std::thread([this, device_handle, capture_interval_ms]()
        {
            uint64_t lastCaptureTimestamp = 0;

            while (m_isCapturing)
            {
                std::cout << "Entering Capturing loop." << std::endl;
                auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                double elapsed = currentTimeInMs - lastCaptureTimestamp;

                if (elapsed >= capture_interval_ms)
                {
                    lastCaptureTimestamp = currentTimeInMs;

                    int nRet = MV_CC_SetCommandValue(device_handle, "TriggerSoftware");
                    if (MV_OK != nRet)
                    {
                        std::cout << "Failed to capture frames via TriggerSoftware. Error code: " << nRet << std::endl;
                        m_logger->log("Error on MV_CC_SetCommandValue(TriggerSoftware): " + std::to_string(nRet), Logger::ERROR);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
                    }
                    else
                    {
                        std::cout << "Capturing frames via TriggerSoftware." << std::endl;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
                std::cout << "Exiting Capturing loop." << std::endl;
            }
        });
    }
}

void MainWindow::start_detection(std::string py_script_path)
{
    std::string py_env = m_py_env_entry->get_text();
    if (py_env.empty())
    {
        std::cerr << "Python environment is not set" << std::endl;
        return;
    }

    std::string model_path = m_load_model_fcb->get_filename();
    if (model_path.empty())
    {
        std::cerr << "model path is not set" << std::endl;
        return;
    }
    
    int patch_size = static_cast<int>(m_patch_size_sb->get_value());
    if (patch_size <= 0)
    {
        std::cerr << "Patch size is not set" << std::endl;
        return;
    }

    double confidence_threshold = m_confidence_threshold_sb->get_value();
    if (confidence_threshold <= 0.0)
    {
        std::cerr << "Confidence threshold is not set" << std::endl;
        return;
    }

    int pixel_threshold = m_pixel_threshold_sb->get_value();
    if (pixel_threshold <= 0)
    {
        std::cerr << "Pixel threshold is not set" << std::endl;
        return;
    }

    std::string result_folder = m_prediction_result_fcb->get_filename();
    if (result_folder.empty())
    {
        std::cerr << "Output dir is not set" << std::endl;
        return;
    }

    // Ensure there's no existing processing thread running
    if (m_processing_thread.joinable()) 
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }

    // Start the frame processing thread
    m_processing_thread = std::thread([this, py_env, py_script_path, model_path, patch_size, confidence_threshold, pixel_threshold, result_folder]()
    {
        FrameData frame_data(nullptr, nullptr); // Initialize FrameData with null pointers

        std::string shm_name = "/my_shared_memory";
        size_t frame_size = 256 * 256;

        // Command to execute the python script
        std::string cmd = py_env + " " + py_script_path +
            std::string(" --model_path ") + model_path +
            std::string(" --shm_name ") + "/my_shared_memory" + 
            std::string(" --frame_size ") + std::to_string(frame_size) +
            std::string(" --frame_width ") + std::to_string(256) +
            std::string(" --frame_height ") + std::to_string(256) +
            std::string(" --patch_size ") + std::to_string(patch_size) +
            std::string(" --confidence_threshold ") + std::to_string(confidence_threshold) +
            std::string(" --pixel_threshold ") + std::to_string(pixel_threshold) +
            std::string(" --output_dir ") + result_folder;

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
        if (ftruncate(shm_fd, frame_size) == -1)
        {
            std::cerr << "Failed to resize shared memory object." << std::endl;
            ::close(shm_fd);
            return;
        }

        // Map shared memory into address space
        std::cout << "Map shared memory" << std::endl;
        void *shm_ptr = mmap(0, frame_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED)
        {
            std::cerr << "Failed to map shared memory." << std::endl;
            ::close(shm_fd);
            return;
        }

        while (m_isCapturing)
        {
            std::cout << "Entering processing loop" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
            if (m_frame_queue.dequeue(frame_data))
            {
                auto frame_metadata = frame_data.pMetadata;
                size_t curr_frame_size = frame_metadata->nFrameLen;

                // Check if the data pointer is valid and frame size is greater than zero
                if (frame_data.pData == nullptr || curr_frame_size == 0) {
                    std::cerr << "Frame data is invalid or has size 0." << std::endl;
                    continue;
                }

                if (curr_frame_size != frame_size)
                {
                    std::cout << "frame size is different" << std::endl;
                    // Resize shared memory object to the data size
                    if (ftruncate(shm_fd, curr_frame_size) == -1)
                    {
                        std::cerr << "Failed to resize shared memory object." << std::endl;
                        continue;
                    }

                    // Map shared memory into address space
                    shm_ptr = mmap(0, curr_frame_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
                    if (shm_ptr == MAP_FAILED)
                    {
                        std::cerr << "Failed to map shared memory." << std::endl;
                        continue;
                    }

                    // Update the command to execute the python script
                    cmd = py_env + " " + py_script_path +
                        std::string(" --model_path ") + model_path +
                        std::string(" --shm_name ") + "/my_shared_memory" + 
                        std::string(" --frame_size ") + std::to_string(frame_metadata->nFrameLen) +
                        std::string(" --frame_width ") + std::to_string(frame_metadata->nWidth) +
                        std::string(" --frame_height ") + std::to_string(frame_metadata->nHeight) +
                        std::string(" --patch_size ") + std::to_string(patch_size) +
                        std::string(" --confidence_threshold ") + std::to_string(confidence_threshold) +
                        std::string(" --pixel_threshold ") + std::to_string(pixel_threshold) +
                        std::string(" --output_dir ") + result_folder;

                    frame_size = curr_frame_size;
                }

                // Copy data to shared memory
                std::cout << "Copy data to shared memory" << std::endl;
                std::memcpy(shm_ptr, frame_data.pData, frame_size);

                // Send command to the python script
                // 1) it will read frame data from the shared memory
                // 2) and run model prediction
                predict(cmd);
            }
            std::cout << "Exiting processing loop" << std::endl;
        }

        // Flush frame queue
        while (!m_frame_queue.isEmpty())
        {
            if (m_frame_queue.dequeue(frame_data))
            {
                size_t curr_frame_size = frame_data.pMetadata->nFrameLen;

                // Check if the data pointer is valid and frame size is greater than zero
                if (frame_data.pData == nullptr || curr_frame_size == 0) {
                    std::cerr << "Frame data is invalid or has size 0." << std::endl;
                    continue;
                }

                // Copy data to shared memory
                std::memcpy(shm_ptr, frame_data.pData, frame_size);

                predict(cmd);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
        }

        // Clean up
        std::cout << "Clean up shared memory object" << std::endl; 
        if (munmap(shm_ptr, frame_size) == -1) // Unmap the shared memory
        {
            std::cerr << "Failed to unmap shared memory." << std::endl;
        }
        ::close(shm_fd);
    });
}

void MainWindow::predict(std::string command)
{
    m_logger->log("Start prediction: " + command);
    
    // Open a pipe to the command
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Failed to run command\n";
        m_logger->log("Unable to run command: " + command, Logger::ERROR);
    }

    // Buffer to hold each line of output
    std::array<char, 256> buffer;

    // Read the output from the pipe line by line
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        std::cout << buffer.data(); // Print each line to the console
        m_logger->log(std::string(buffer.data()));
    }

    // Close the pipe
    int returnCode = pclose(pipe);
    if (returnCode != 0)
    {
        std::cerr << "Command failed with return code " << returnCode << std::endl;
        m_logger->log("Unable to close the pipe: " + std::to_string(returnCode), Logger::ERROR);
    }

    m_logger->log("End prediction");
}

void MainWindow::stop_capture(void *device_handle)
{
    if (m_capturing_thread.joinable())
    {
        m_capturing_thread.join();  // Wait for the thread to complete
    }

    // Ensure there's no existing processing thread running
    if (m_processing_thread.joinable()) 
    {
        m_processing_thread.join();  // Wait for previous thread to finish
    }

    int nRet = MV_CC_StopGrabbing(device_handle);
    if (nRet != MV_OK)
    {
        std::cerr << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StopGrabbing", Logger::ERROR);
    }
}