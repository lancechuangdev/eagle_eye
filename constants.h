#ifndef EAGLE_EYE_CONSTANTS_H
#define EAGLE_EYE_CONSTANTS_H

// #include <cstddef> // For size_t
#include <string>

// App-related constants
const std::string APP_NAME = "Eagle Eye";

// Detection-related constants
const int WARMUP_NUM_ITERATIONS = 5;
const double DEFAULT_DETECTION_RATE = 20.0;
const int DETECTION_TIMEOUT_MS = 3000;
const int FRAME_BATCH_SIZE = 2;
const int MAX_PATCHES_PER_BATCH = 50; // The max number of anomaly patches that detection model generates per batch.

// RGB-related constants
const int RGB_CHANNELS = 3;
const int RGBA_CHANNELS = 4;

// Patch-related constants
const int PATCH_SIZE = 512; // The patch_size that detection expects.

// Project-related constants
const std::string TEMP_PROJECT_NAME = "ad-hoc";

#endif // EAGLE_EYE_CONSTANTS_H
