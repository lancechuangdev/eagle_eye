import os
import sys
import argparse
import posix_ipc
import mmap
import numpy as np
import tensorflow as tf
from PIL import Image

def read_from_shared_memory(shm_name, frame_size):
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

    # Return the frame as a numpy array
    return frame_array

def main():
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

    if frame_width < patch_size or frame_height < patch_size:
        print("Error: frame width or height is smaller than the specified patch size.")
        sys.exit(1)

    # Read the frame from shared memory
    frame = read_from_shared_memory(shm_name, frame_size)

    # Reshape and preprocess the grayscale frame
    frame = frame.reshape((patch_size, frame_width))

    # Normalize frame to [0, 1] range
    frame = frame.astype(np.float32) / 255.0

    # Load the model
    model = tf.keras.models.load_model(model_path)

    # Extract and reshape each patch_size*patch_size patch
    num_patches = frame_width // patch_size
    batch = []
    for i in range(num_patches):
        patch = frame[:, i * patch_size:(i + 1) * patch_size]
        patch = patch.reshape((patch_size, patch_size, 1))  # Reshape to (256, 256, 1)
        batch.append(patch)

    # Handle the remaining part as a smaller patch, if any
    remainder = frame_width % patch_size
    if remainder > 0:
        last_patch = frame[:, num_patches * patch_size:]
        # Pad the last patch to patch_size*patch_size
        last_patch = np.pad(last_patch, ((0, 0), (0, patch_size - remainder)), mode='constant') # This fills in the additional columns with zeros, creating a uniform background
        last_patch = last_patch.reshape((patch_size, patch_size, 1))
        batch.append(last_patch)

    # Convert list to numpy array with batch shape (num_patches, patch_size, patch_size, 1)
    frame_batch = np.array(batch)

    # Perform prediction on the entire batch
    predictions = model.predict(frame_batch)
    predictions = (predictions > confidence_threshold).astype(np.float32)

    # Check each prediction for anomaly
    for i, (prediction, patch) in enumerate(zip(predictions, batch)):
        # Threshold check for anomalies
        anomaly_count = np.sum(prediction == 1)
        print(f"Patch {i}: Anomaly detected with count = {anomaly_count}")
        
        if anomaly_count > pixel_threshold:
            # Convert arrays to image format
            patch_image = Image.fromarray((patch.squeeze() * 255).astype(np.uint8), mode='L')  # Convert to grayscale image
            prediction_image = Image.fromarray((prediction.squeeze() * 255).astype(np.uint8), mode='L')

            # Save the images
            patch_filename = os.path.join(output_dir, f"patch_{i}.png")
            prediction_filename = os.path.join(output_dir, f"prediction_{i}.png")
            
            patch_image.save(patch_filename)
            prediction_image.save(prediction_filename)

            print(f"Saved patch {i} and its prediction mask as images.")

if __name__ == "__main__":
    main()