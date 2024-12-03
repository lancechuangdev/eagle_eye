#!/usr/bin/env python
# coding: utf-8

import os
import posix_ipc
import mmap
import numpy as np
import tensorflow as tf
from PIL import Image
from datetime import datetime
import json
from websocket_server import WebsocketServer
import concurrent.futures

shared_memory_name = '/ee_shared_memory' # DONOT CHANGE
patch_size = 256
home_dir = os.path.expanduser("~")
output_dir = os.path.join(home_dir, "eagle_eye", "detection_results")
os.makedirs(output_dir, exist_ok=True)
executor = concurrent.futures.ThreadPoolExecutor() # ThreadPoolExecutor for saving files

def print_with_ts(message):
    print(f"{datetime.now():%Y-%m-%d %H:%M:%S.%f} - {message}")

def read_frame_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset):
    """
    Reads a single frame from the shared memory based on the frame information.

    Parameters:
    - shm_name: The name of the shared memory.
    - frame_size: The frame size.
    - frame_width: The frame width.
    - frame_height: The frame height.
    - offset: The frame offset.

    Returns:
    - bytes: The data of the frame read from shared memory.
    """

    # Open the shared memory object with read access
    shm = posix_ipc.SharedMemory(shm_name)

    # Align the offset to the system page size and calculate the size needed for mmap
    aligned_offset = offset - (offset % mmap.PAGESIZE)
    intra_frame_offset = offset % mmap.PAGESIZE
    map_size = frame_size + intra_frame_offset

    # Memory-map the shared memory object
    with mmap.mmap(shm.fd, map_size, mmap.MAP_SHARED, mmap.ACCESS_READ, offset=aligned_offset) as mmap_obj:
        # Move to the intra-frame offset within the mapped memory
        mmap_obj.seek(intra_frame_offset)
        # Read the frame data
        frame_data = mmap_obj.read(frame_size)
    
    # Close the shared memory descriptor
    shm.close_fd()

    # Convert the bytes to a numpy array
    frame_data = np.frombuffer(frame_data, dtype=np.uint8)
    print(f"frame_data shape: {frame_data.shape}")
    
    # Reshape the array based on the frame dimensions
    frame = frame_data.reshape((frame_height, frame_width))
    print(f"frame shape: {frame.shape}")
    
    return frame

def read_frames_from_shared_memory(shm_name, frame_width, frame_height, frames_array):
    frames = []

    for frame_data in frames_array:
        frame_size = frame_data['frame_size']
        offset = frame_data['offset']
        serial_number = frame_data['serial_number']

        if frame_height % patch_size != 0:
            print(f"Error: frame height {frame_height} is not a multiple of patch size {patch_size}.")
            continue
        if frame_width < patch_size or frame_height < patch_size:
            print("Error: frame width or height is smaller than the specified patch size.")
            continue
        if frame_width * frame_height != frame_size:
            print("Error: frame width x height is not equal to the frame size.")
            continue

        # Read the frame from shared memory
        frame = read_frame_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset)

        frames.append((frame, serial_number))
    
    return frames

def build_batch_from_frames(frames, frame_width, frame_height, channels):
    batch = []
    for frame, _ in frames:
        # Normalize frame to [0, 1] range
        frame = frame.astype(np.float32) / 255.0

        # Convert grayscale to RGB if needed
        if frame.ndim == 2 and channels == 3:
            frame = np.stack([frame, frame, frame], axis=-1)  # Convert to (H, W, 3)

        # Extract and reshape each patch_size*patch_size patch
        num_patches_x = frame_width // patch_size
        print(f"num_patches_x: {num_patches_x}")
        num_patches_y = frame_height // patch_size
        print(f"num_patches_y: {num_patches_y}")

        for j in range(num_patches_y):
            for i in range(num_patches_x):
                patch = frame[j * patch_size:(j + 1) * patch_size, i * patch_size:(i + 1) * patch_size]
                patch = patch.reshape((patch_size, patch_size, channels))  # Reshape to (patch_size, patch_size, channels)
                batch.append(patch)

            # Handle the remaining part as a smaller patch, if any
            remainder = frame_width % patch_size
            # print(f"remainder: {remainder}")

            if remainder > 0:
                # Adjust the start position for the last patch so it aligns properly
                start_x = frame_width - patch_size
                last_patch = frame[j * patch_size:(j + 1) * patch_size, start_x:start_x + patch_size]
                last_patch = last_patch.reshape((patch_size, patch_size, channels))
                batch.append(last_patch)
    return batch

def get_welcome_message():
    return (f"Welcome to Eagle Eye Anomaly Detection Service!\n"
            f"Configuration:\n"
            f"Patch Size: {patch_size}\n"
            f"Output Directory: {output_dir}\n")

# Called for every client connecting (after handshake)
def new_client(client, server):
    print_with_ts("New client connected and was given id %d" % client['id'])
    welcome_message = get_welcome_message()
    print_with_ts(welcome_message)
    
# Called for every client disconnecting
def client_left(client, server):
    print_with_ts("Client(%d) disconnected" % client['id'])

def save_images_and_transaction(output_path, frame_images, frames_metadata, prediction_images, predictions_metadata, transaction_json):
    """Save frames, predictions, and transaction JSON in a separate thread."""
    try:
        # Ensure the directory exists
        os.makedirs(output_path, exist_ok=True)

        # Save frames
        for i, metadata in enumerate(frames_metadata):
            file_name = metadata['file_name']
            image = frame_images[i]
            image.save(file_name)

        # Save predictions
        for i, metadata in enumerate(predictions_metadata):
            file_name = metadata['file_name']
            image = prediction_images[i]
            image.save(file_name)

        # Write transaction JSON
        with open(os.path.join(output_path, "transaction_data.json"), "w") as trans_json_file:
            json.dump(transaction_json, trans_json_file, indent=4)

        print_with_ts(f"Transaction {transaction_json['transaction_id']} files saved successfully.\n")
    except Exception as e:
        print_with_ts(f"Error saving transaction {transaction_json['transaction_id']} files: {e}\n")

# Called when a client sends a message
def message_received(client, server, message):
    print_with_ts("Receiving a message from Client(%d): %s" % (client['id'], message))
    data = json.loads(message)
    frames_array = data.get('frames', [])
    transaction_id = data.get('transaction_id', 0)
    confidence_threshold = data.get('confidence_threshold', 0.8)
    # print(f'confidence_threshold: {confidence_threshold}')
    pixel_threshold = data.get('pixel_threshold', 0.03)
    pixel_threshold = pixel_threshold * patch_size * patch_size
    # print(f'pixel_threshold: {pixel_threshold}')
    frame_width = data.get('frame_width', 0)
    frame_height = data.get('frame_height', 0)
    total_anomalies = 0
    output_path = os.path.join(output_dir, transaction_id)
    serial_numbers = []

    # Build the initial transaction json object
    transaction_json = {
        "transaction_id": transaction_id,
        "patch_size": patch_size,
        "confidence_threshold": confidence_threshold,
        "pixel_threshold": pixel_threshold,
        "frame_width": frame_width,
        "frame_height": frame_height
    }

    if frames_array and transaction_id:
        # Notify client about prediction start
        print_with_ts(f"Transaction {transaction_id} started: Starting prediction on {len(frames_array)} image(s)\n")

        # Read frames from shared memory
        frames = read_frames_from_shared_memory(shared_memory_name, frame_width, frame_height, frames_array)
        num_frames = len(frames)
        transaction_json["num_frames"] = num_frames

        # Build batch from frames
        batch = build_batch_from_frames(frames, frame_width, frame_height, 3)

        if batch:
            num_patches = len(batch)
            transaction_json["num_patches"] = num_patches
            patches_per_frame = num_patches / num_frames

            # Convert list to numpy array with batch shape (num_patches, patch_size, patch_size, channels)
            frame_batch = np.array(batch)
            print(f"frame_batch shape: {frame_batch.shape}")
            
            # Create an array of original indices
            original_indices = np.arange(num_patches)

            # Perform binary classification on the entire batch
            print_with_ts("Binary Classification Started.\n")
            mobilenet_predictions = mobilenet_v2.predict(frame_batch)
            print_with_ts("Binary Classification Stopped.\n")
            print(f"Mobilenet_v2 predictions shape: {mobilenet_predictions.shape}")

            # Get boolean mask for patches exceeding the confidence threshold
            confident_mask = np.squeeze(mobilenet_predictions > confidence_threshold)
            print(f"Confident mask: {confident_mask}")

            # Perform prediction on the anomaly patches
            frame_batch = frame_batch[confident_mask]
            original_indices = original_indices[confident_mask]  # Track original indices
            frame_batch = frame_batch[..., 0:1]  # Extract the red channel
            if frame_batch.size > 0:
                print_with_ts("Unet Prediction Started.\n")
                predictions = unet.predict(frame_batch)
                print_with_ts("Unet Prediction Stopped.\n")
                predictions = (predictions > 0.5).astype(np.float32)
                print(f"Unet predictions shape: {predictions.shape}")
                
                frames_metadata = []
                frame_images = []
                predictions_metadata = []
                prediction_images = []
                prediction_frame_ids = []
                
                # Check each prediction for anomaly
                for i, prediction in enumerate(predictions):
                    original_idx = int(original_indices[i])  # Convert NumPy int64 to Python int
                    # Threshold check for anomalies
                    anomaly_count = np.sum(prediction == 1)
                    
                    # Print per-patch anomaly count
                    print_with_ts(f"Patch {original_idx}: {anomaly_count} anomaly pixels detected w/ confidence threshold 0.5.\n")
                    
                    if anomaly_count >= pixel_threshold:
                        # Convert arrays to image format
                        prediction_image = Image.fromarray((prediction.squeeze() * 255).astype(np.uint8), mode='L')
                        prediction_images.append(prediction_image)

                        # Build the prediction metadata
                        predictions_metadata.append({
                            "prediction_id": original_idx, 
                            "file_name": os.path.join(output_path, f"prediction_{original_idx}.png")
                        })

                        # Store the corresponding frame id for the anomaly patch
                        prediction_frame_id = original_idx // patches_per_frame
                        if prediction_frame_id not in prediction_frame_ids:
                            prediction_frame_ids.append(prediction_frame_id)

                        total_anomalies += 1

                if total_anomalies > 0:
                    for i, (frame, serial_number) in enumerate(frames):
                        frame_images.append(Image.fromarray(frame.astype(np.uint8)))

                        frames_metadata.append({
                            "frame_id": i,
                            "serial_number": serial_number,
                            "file_name": os.path.join(output_path, f"frame_{i}.png"),
                        })

                        if i in prediction_frame_ids and serial_number not in serial_numbers:
                            serial_numbers.append(serial_number)

                    transaction_json["frames"] = frames_metadata
                    transaction_json["predictions"] = predictions_metadata
                    transaction_json["total_anomalies"] = total_anomalies

                    # Dispatch saving task to another thread
                    executor.submit(save_images_and_transaction, output_path, frame_images, frames_metadata, prediction_images, predictions_metadata, transaction_json)

                # Print a final summary of the prediction results
                print_with_ts(f"Transaction {transaction_id} completed: {total_anomalies} patches with anomalies are more than the detection threshold.\n")

    result_json = {
        "transaction_id": transaction_id,
        "status": "complete",
        "total_anomalies": total_anomalies,
        "serial_numbers": serial_numbers
    }
    result = json.dumps(result_json)
    server.send_message(client, result)

# Load models
model_path = '/usr/local/share/eagle_eye/ee.h5'
mobilenet_v2 = tf.keras.models.load_model(model_path)

# BCE w/ Intersection over Union (IoU)
def iou(y_true, y_pred):
    y_pred = tf.round(y_pred)
    intersection = tf.reduce_sum(y_true * y_pred)
    total = tf.reduce_sum(y_true + y_pred)
    union = total - intersection
    iou = intersection / (union + tf.keras.backend.epsilon())
    return iou

custom_objects = { 'iou': iou }
model_path = '/usr/local/share/eagle_eye/ds.keras'
unet = tf.keras.models.load_model(model_path, custom_objects=custom_objects)

PORT=9001 # DONOT CHANGE
server = WebsocketServer(port = PORT)
server.set_fn_new_client(new_client)
server.set_fn_client_left(client_left)
server.set_fn_message_received(message_received)
server.run_forever()




