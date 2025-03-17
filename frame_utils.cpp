#include <opencv2/opencv.hpp>
#include "frame_utils.h"
#include "constants.h"

std::vector<std::vector<float>> FrameUtils::build_batch(const std::vector<FrameData>& frames)
{
    std::vector<std::vector<float>> batch_patches;

    for (const auto& frame : frames) {
        int width = frame.pMetadata->nWidth;
        int height = frame.pMetadata->nHeight;
        cv::Mat img;

        switch(frame.pMetadata->enPixelType)
        {
            case PixelType_Gvsp_BayerGB8: {
                // Load as a single-channel image (because it's a raw Bayer pattern)
                cv::Mat bayer_img(height, width, CV_8UC1, frame.pData);
                // Convert Bayer pattern to RGB using OpenCV's demosaicing
                cv::cvtColor(bayer_img, img, cv::COLOR_BayerGB2RGB);
                break;
            }
            case PixelType_Gvsp_Mono8: {
                // If the format is Mono8 (grayscale), convert to cv::Mat (1 channel)
                cv::Mat gray_img = cv::Mat(height, width, CV_8UC1, frame.pData);
                // Convert to RGB (3 channels) for the model
                cv::cvtColor(gray_img, img, cv::COLOR_GRAY2RGB);
                break;
            }
            default:
                std::cerr << "Unsupported pixel format!" << std::endl;
                continue;  // Skip this frame if the format is unsupported
        }

        // pData is RGB and each pixel is represented by 3 bytes (BGR or RGB)
        cv::Mat rgb(height, width, CV_8UC3, frame.pData);

        // Normalize (convert to float [0,1])
        img.convertTo(img, CV_32F, 1.0 / 255.0);

        // Split into patches
        for (int y = 0; y < height; y += PATCH_SIZE) {
            for (int x = 0; x < width; x += PATCH_SIZE) {
                // Extract patch
                cv::Mat patch = img(cv::Rect(x, y, PATCH_SIZE, PATCH_SIZE));

                // Convert from HWC to CHW format
                std::vector<float> patch_data(PATCH_SIZE * PATCH_SIZE * RGB_CHANNELS);
                std::vector<cv::Mat> chw_channels(3);

                // Split into three separate channels
                cv::split(patch, chw_channels);

                // Copy each channel separately to patch_data
                for (int c = 0; c < 3; ++c) {
                    std::memcpy(patch_data.data() + c * PATCH_SIZE * PATCH_SIZE,
                                chw_channels[c].data,
                                PATCH_SIZE * PATCH_SIZE * sizeof(float));
                }

                batch_patches.push_back(std::move(patch_data));
            }
        }
    }

    return batch_patches;  // Each element is a (3, 512, 512) float vector
}


Ort::Value FrameUtils::create_input_tensor(const std::vector<std::vector<float>>& batch_patches) {
    const int batch_size = batch_patches.size();
    const int num_elements = batch_size * RGB_CHANNELS * PATCH_SIZE * PATCH_SIZE;

    // Flatten batch into a single vector
    std::vector<float> input_tensor_values;
    input_tensor_values.reserve(num_elements);
    for (const auto& patch : batch_patches) {
        input_tensor_values.insert(input_tensor_values.end(), patch.begin(), patch.end());
    }

    // Define input tensor shape (N, C, H, W)
    std::array<int64_t, 4> input_shape = {batch_size, RGB_CHANNELS, PATCH_SIZE, PATCH_SIZE};

    // Create an ONNX tensor
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);
    // Ort::MemoryInfo memory_info("Cuda", OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemTypeDefault);

    return Ort::Value::CreateTensor<float>(
        memory_info,
        input_tensor_values.data(),
        input_tensor_values.size(),
        input_shape.data(),
        input_shape.size()
    );
}

Ort::Value FrameUtils::create_input_tensor(uint8_t* pData, int frame_width, int frame_height, int patch_size) {
    const int num_channels = 3; // Model expects RGB
    cv::Mat image(frame_height, frame_width, CV_8UC3, pData);
    cv::Mat image_float;
    
    // Normalize to [0,1] range
    image.convertTo(image_float, CV_32F, 1.0 / 255.0);

    std::vector<std::vector<float>> batch_patches;
    
    // Split into PATCH_SIZE x PATCH_SIZE patches
    for (int y = 0; y < frame_height; y += patch_size) {
        for (int x = 0; x < frame_width; x += patch_size) {
            // Ensure patch does not go out of bounds
            if (x + patch_size > frame_width || y + patch_size > frame_height) continue;

            // Extract patch
            cv::Mat patch = image_float(cv::Rect(x, y, patch_size, patch_size));

            // Convert HWC -> CHW
            std::vector<float> patch_data(patch_size * patch_size * num_channels);
            std::vector<cv::Mat> chw_channels(num_channels);
            cv::split(patch, chw_channels);

            for (int c = 0; c < num_channels; ++c) {
                std::memcpy(patch_data.data() + c * patch_size * patch_size,
                            chw_channels[c].data,
                            patch_size * patch_size * sizeof(float));
            }

            batch_patches.push_back(std::move(patch_data));
        }
    }

    // Flatten patches into a single vector
    int batch_size = batch_patches.size();
    std::vector<float> input_tensor_values;
    input_tensor_values.reserve(batch_size * num_channels * patch_size * patch_size);
    for (const auto& patch : batch_patches) {
        input_tensor_values.insert(input_tensor_values.end(), patch.begin(), patch.end());
    }

    // Define input tensor shape (N, C, H, W)
    std::array<int64_t, 4> input_shape = {batch_size, num_channels, patch_size, patch_size};

    // Create ONNX Tensor
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);
    // Ort::MemoryInfo memory_info("Cuda", OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemTypeDefault);

    return Ort::Value::CreateTensor<float>(
        memory_info,
        input_tensor_values.data(),
        input_tensor_values.size(),
        input_shape.data(),
        input_shape.size()
    );
}