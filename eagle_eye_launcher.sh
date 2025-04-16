#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/onnxruntime-linux-x64-gpu-1.20.1/lib:/usr/local/TensorRT-10.8.0.43/lib:/usr/local/cuda-12.8/lib64:/opt/MVS/lib/64:/opt/MVS/lib/32:/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
exec /usr/local/bin/eagle_eye/ee