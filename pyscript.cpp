#include "pyscript.h"

const std::string unet_predict_py = R"(
import os
import sys
import argparse
import posix_ipc
import mmap
import numpy as np
import tensorflow as tf
from PIL import Image
from datetime import datetime

def read_from_shared_memory(shm_name, frame_size, frame_width, frame_height):    
    # Open the shared memory object
    shm = posix_ipc.SharedMemory(shm_name)

    # Map the shared memory object into Python's address space
    with mmap.mmap(shm.fd, frame_size, mmap.MAP_SHARED, mmap.PROT_READ) as mmap_obj:
        # Read the data from the memory map
        frame_bytes = mmap_obj.read(frame_size)

    # Close the shared memory object
    shm.close_fd()

    # Convert the bytes to a numpy array
    frame_array = np.frombuffer(frame_bytes, dtype=np.uint8)
    print(f"frame_array shape: {frame_array.shape}")
    
    # Reshape the array based on the frame dimensions
    frame = frame_array.reshape((frame_height, frame_width))
    print(f"frame shape: {frame.shape}")
    
    return frame

# BCE w/ Intersection over Union (IoU)
def iou(y_true, y_pred):
    y_pred = tf.round(y_pred)
    intersection = tf.reduce_sum(y_true * y_pred)
    total = tf.reduce_sum(y_true + y_pred)
    union = total - intersection
    iou = intersection / (union + tf.keras.backend.epsilon())
    return iou

def main():
    print("main started")

    parser = argparse.ArgumentParser(description='Process frame from shared memory.')
    parser.add_argument('--shm_name', type=str, required=True, help='Shared memory name')
    parser.add_argument('--frame_size', type=int, required=True, help='Size of the frame in bytes')
    parser.add_argument('--frame_width', type=int, required=True, help='Width of the frame')
    parser.add_argument('--frame_height', type=int, required=True, help='Height of the frame')
    parser.add_argument('--model_path', type=str, required=True, help='Model path')
    parser.add_argument('--patch_size', type=int, required=True, help='Patch size for model')
    parser.add_argument('--confidence_threshold', type=float, required=True, help='Confidence threshold for mask prediction')
    parser.add_argument('--pixel_threshold', type=float, required=True, help='Anomaly detection area (in pixel) threshold for mask prediction')
    parser.add_argument('--output_dir', type=str, required=True, help='Model prediction output directory')

    args = parser.parse_args()

    shm_name = args.shm_name
    frame_size = args.frame_size
    frame_width = args.frame_width
    frame_height = args.frame_height
    model_path = args.model_path
    patch_size = args.patch_size
    confidence_threshold = args.confidence_threshold
    pixel_threshold = args.pixel_threshold
    output_dir = args.output_dir

    print(f"frame size: {frame_size}, frame_width: {frame_width}, frame_height: {frame_height}")
    print(f"share memory name: {shm_name}, model_path: {model_path}, patch_size: {patch_size}, confidence_threshold: {confidence_threshold}, pixel_threshold: {pixel_threshold}, output_dir: {output_dir}")

    if frame_width < patch_size or frame_height < patch_size:
        print("Error: frame width or height is smaller than the specified patch size.")
        sys.exit(1)

    # Read the frame from shared memory
    frame = read_from_shared_memory(shm_name, frame_size, frame_width, frame_height)

    # Reshape and preprocess the grayscale frame
    reshaped_frame = frame[:patch_size, :]
    print(f"reshaped_frame shape: {reshaped_frame.shape}")

    # Normalize frame to [0, 1] range
    reshaped_frame = reshaped_frame.astype(np.float32) / 255.0

    # Define the custom objects dictionary
    custom_objects = {
        'iou': iou
    }
    model = tf.keras.models.load_model(model_path, custom_objects=custom_objects)
    print(f"model loaded")

    # Extract and reshape each patch_size*patch_size patch
    num_patches = frame_width // patch_size
    print(f"num_patches: {num_patches}")
    batch = []
    for i in range(num_patches):
        patch = reshaped_frame[:, i * patch_size:(i + 1) * patch_size]
        patch = patch.reshape((patch_size, patch_size, 1))  # Reshape to (patch_size, patch_size, 1)
        batch.append(patch)

    # Handle the remaining part as a smaller patch, if any
    remainder = frame_width % patch_size
    print(f"remainder: {remainder}")

    if remainder > 0:
        last_patch = reshaped_frame[:, num_patches * patch_size:]
        # Pad the last patch to patch_size*patch_size
        last_patch = np.pad(last_patch, ((0, 0), (0, patch_size - remainder)), mode='constant') # This fills in the additional columns with zeros, creating a uniform background
        last_patch = last_patch.reshape((patch_size, patch_size, 1))
        batch.append(last_patch)

    # Convert list to numpy array with batch shape (num_patches, patch_size, patch_size, 1)
    frame_batch = np.array(batch)
    print(f"frame_batch shape: {frame_batch.shape}")

    # Perform prediction on the entire batch
    predictions = model.predict(frame_batch)
    predictions = (predictions > confidence_threshold).astype(np.float32)
    print(f"predictions shape: {predictions.shape}")

    # Check each prediction for anomaly
    for i, (prediction, patch) in enumerate(zip(predictions, batch)):
        # Threshold check for anomalies
        anomaly_count = np.sum(prediction == 1)
        print(f"Patch {i}: Anomaly detected with count = {anomaly_count} px")
        
        if anomaly_count > pixel_threshold:
            # Convert arrays to image format
            patch_image = Image.fromarray((patch.squeeze() * 255).astype(np.uint8), mode='L')  # Convert to grayscale image
            prediction_image = Image.fromarray((prediction.squeeze() * 255).astype(np.uint8), mode='L')

            # Save the images
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            patch_filename = os.path.join(output_dir, f"patch_{i}_{timestamp}.png")
            prediction_filename = os.path.join(output_dir, f"prediction_{i}_{timestamp}.png")
            
            patch_image.save(patch_filename)
            prediction_image.save(prediction_filename)

            print(f"Saved patch {i} and its prediction mask as images.")

if __name__ == "__main__":
    main()
)";