#include "main_window.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder)
    : Gtk::Window(obj),
      m_builder(refBuilder)
{
    signal_show().connect(sigc::mem_fun(*this, &MainWindow::on_window_shown));

    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("main_window", root);
    root->set_title("Eagle Eye");

    m_builder->get_widget("capture_source_cbox", m_cameraComboBox);
    if (m_cameraComboBox)
    {
        m_cameraComboBox->signal_changed().connect([this]() 
        {
            auto selectedCamera = m_cameraComboBox->get_active_text();
            // auto folder = m_capturePickerFcb->get_filename();
            // m_startCaptureBtn->set_sensitive(!folder.empty() && !selectedCamera.empty()); 
        });
    }

    m_builder->get_widget("capture_rate_sb", m_captureRateSb);

    m_builder->get_widget("load_model_fcb", m_load_model_fcb);
    if (m_load_model_fcb)
    {
        m_load_model_fcb->signal_selection_changed().connect([this]()
        {
            auto model = m_load_model_fcb->get_filename();
        });
    }

    m_builder->get_widget("prediction_result_fcb", m_prediction_result_fcb);
    if (m_prediction_result_fcb)
    {
        m_prediction_result_fcb->signal_selection_changed().connect([this]()
        {
            auto result_folder = m_prediction_result_fcb->get_filename();
        });
    }

    m_builder->get_widget("preflight_btn", m_preflight_btn);
    if (m_preflight_btn)
    {
        m_preflight_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_preflight_clicked));
    }

    m_builder->get_widget("start_btn", m_start_btn);
    if (m_start_btn)
    {
        m_start_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_start_clicked));
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
                m_cameraComboBox->append(std::string((char *)serialNumber));
            }
        }
        m_cameraComboBox->append("All Cameras");
    }
    else
    {
        std::cout << "No camera found." << std::endl;
        // m_logger->log("No camera found.");
    }
}

void MainWindow::discover_cameras()
{
    // enum device
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE, &m_camList);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_EnumDevices fail! Error code: " << nRet << std::endl;
        // m_logger->log("Error on MV_CC_EnumDevices: " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::on_preflight_clicked()
{

}

void MainWindow::on_start_clicked()
{
    double captureIntervalMs = 0.0;

    if (m_captureRateSb)
    {
        int captureRate = m_captureRateSb->get_value();
        // Calculate the capture interval (in milliseconds) based on capture rate (FPS)
        captureIntervalMs = 1000.0 / static_cast<double>(captureRate);
    }
}