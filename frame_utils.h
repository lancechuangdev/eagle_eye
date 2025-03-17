#ifndef EAGLE_EYE_FRAME_UTILS_H
#define EAGLE_EYE_FRAME_UTILS_H

#include <vector>
#include <onnxruntime_cxx_api.h>
#include "frame_data.h"

class FrameUtils
{
public:
    static std::vector<std::vector<float>> build_batch(const std::vector<FrameData>& frames);
    static Ort::Value create_input_tensor(const std::vector<std::vector<float>>& batch_patches);
    static Ort::Value create_input_tensor(uint8_t* pData, int frame_width, int frame_height, int patch_size);
};

#endif // EAGLE_EYE_FRAME_UTILS_H