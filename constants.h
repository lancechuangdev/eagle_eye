#ifndef EAGLE_EYE_CONSTANTS_H
#define EAGLE_EYE_CONSTANTS_H

// #include <cstddef> // For size_t
#include <string>

// App-related constants
const std::string APP_NAME = "Eagle Eye";

// Detection-related constants
const double CAPTURE_RATE = 100.0;
const int WARMUP_NUM_ITERATIONS = 5;
const int DETECTION_TIMEOUT_MS = 3000;
const int FRAME_BATCH_SIZE = 2;
const int MAX_PATCHES_PER_BATCH = 50; // The max number of anomaly patches that detection model generates per batch.

// RGB-related constants
const int RGB_CHANNELS = 3;
const int RGBA_CHANNELS = 4;

// Patch-related constants
const int PATCH_SIZE = 512; // The patch_size that detection expects.

// Model-related constants
const std::string MODEL_PATH = "/usr/local/share/eagle_eye/model.onnx";
// const std::string MODEL_CACHE_PATH = "/usr/local/share/eagle_eye/engine_cache";

// Project-related constants
const std::string TEMP_PROJECT_NAME = "ad-hoc";

#endif // EAGLE_EYE_CONSTANTS_H
