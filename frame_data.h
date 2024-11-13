#ifndef EAGLE_EYE_FRAME_DATA_H
#define EAGLE_EYE_FRAME_DATA_H

#include "MvCameraControl.h"

struct FrameData {
    unsigned char* pData;
    MV_FRAME_OUT_INFO_EX* pMetadata;
    std::string serial_number;

    // Default constructor
    FrameData() : pData(nullptr), pMetadata(nullptr), serial_number("") {}

    // Constructor
    FrameData(unsigned char* data, MV_FRAME_OUT_INFO_EX* metadata, std::string sn)
        : pData(data), pMetadata(metadata),  serial_number(sn) {}
};

#endif // EAGLE_EYE_FRAME_DATA_H