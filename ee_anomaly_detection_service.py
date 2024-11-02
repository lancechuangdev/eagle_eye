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

shared_memory_name = '/ee_shared_memory' # DONOT CHANGE
model_path = '/usr/local/share/eagle_eye/ds.keras'
patch_size = 256
confidence_threshold = 0.8
pixel_threshold = 2000
home_dir = os.path.expanduser("~")
output_dir = os.path.join(home_dir, "eagle_eye", "test_result")
os.makedirs(output_dir, exist_ok=True)

def print_with_ts(message):
    print(f"{datetime.now():%Y-%m-%d %H:%M:%S.%f} - {message}")

def read_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset):
    """
    Reads a single frame from the shared memory based on the frame information.

    Parameters:
    - shm_name: The name of the shared memory.
    - frame_info: Dictionary containing 'offset' and 'frame_size' for the desired frame.

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

def build_batch_from_frames(frames, patch_size, shared_memory_name):
    batch = []
    for frame_data in frames:
        frame_size = frame_data['frame_size']
        frame_height = frame_data['frame_height']
        frame_width = frame_data['frame_width']
        offset = frame_data['offset']

        if frame_width * frame_height != frame_size:
            print("Error: frame width x height is not equal to the frame size.")
            continue
            
        if frame_width < patch_size or frame_height < patch_size:
            print("Error: frame width or height is smaller than the specified patch size.")
            continue
        
        # Read the frame from shared memory
        frame = read_from_shared_memory(shared_memory_name, frame_size, frame_width, frame_height, offset)

        # Reshape and preprocess the grayscale frame
        reshaped_frame = frame[:patch_size, :]
        print(f"reshaped_frame shape: {reshaped_frame.shape}")

        # Convert the reshaped frame to an image
        #reshaped_image = Image.fromarray(reshaped_frame.astype(np.uint8))
        
        # Save the image to a file
        #timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
        #orig_filename = os.path.join(output_dir, f"orig_{timestamp}.png")
        #reshaped_image.save(orig_filename)
        
        # Normalize frame to [0, 1] range
        reshaped_frame = reshaped_frame.astype(np.float32) / 255.0

        # Extract and reshape each patch_size*patch_size patch
        num_patches = frame_width // patch_size
        # print(f"num_patches: {num_patches}")
        for i in range(num_patches):
            patch = reshaped_frame[:, i * patch_size:(i + 1) * patch_size]
            patch = patch.reshape((patch_size, patch_size, 1))  # Reshape to (patch_size, patch_size, 1)
            batch.append(patch)

        # Handle the remaining part as a smaller patch, if any
        remainder = frame_width % patch_size
        # print(f"remainder: {remainder}")

        if remainder > 0:
            # Adjust the start position for the last patch so it aligns properly
            start_x = frame_width - patch_size
            last_patch = reshaped_frame[:, start_x:start_x + patch_size]
            last_patch = last_patch.reshape((patch_size, patch_size, 1))
            batch.append(last_patch)
    
    return batch

def get_welcome_message():
    return (f"Welcome to Eagle Eye Anomaly Detection Service!\n"
            f"Configuration:\n"
            f"Model Path: {model_path}\n"
            f"Patch Size: {patch_size}\n"
            f"Confidence Threshold: {confidence_threshold}\n"
            f"Pixel Threshold: {pixel_threshold}\n"
            f"Output Directory: {output_dir}\n")

# Called for every client connecting (after handshake)
def new_client(client, server):
    print_with_ts("New client connected and was given id %d" % client['id'])
    welcome_message = get_welcome_message()
    print_with_ts(welcome_message)
    
# Called for every client disconnecting
def client_left(client, server):
    print_with_ts("Client(%d) disconnected" % client['id'])

# Called when a client sends a message
def message_received(client, server, message):
    print_with_ts("Receiving a message from Client(%d): %s" % (client['id'], message))
    data = json.loads(message)
    frames = data.get('frames', [])

    if frames:
        # Notify client about prediction start
        initial_message = f"Starting prediction on {len(frames)} image(s)...\n"
        
        # Build batch from frames
        batch = build_batch_from_frames(frames, patch_size, shared_memory_name)
        
        if batch:
            # Convert list to numpy array with batch shape (num_patches, patch_size, patch_size, 1)
            frame_batch = np.array(batch)
            # print(f"frame_batch shape: {frame_batch.shape}")
            
            # Perform prediction on the entire batch
            predictions = model.predict(frame_batch)
            predictions = (predictions > confidence_threshold).astype(np.float32)
            # print(f"predictions shape: {predictions.shape}")
            
            total_anomalies = 0
            
            # List to accumulate messages to send to the client
            client_messages = [initial_message]
            
            # Check each prediction for anomaly
            for i, (prediction, patch) in enumerate(zip(predictions, batch)):
                # Threshold check for anomalies
                anomaly_count = np.sum(prediction == 1)
                
                # Accumulate per-patch anomaly count in client_messages
                client_messages.append(f"Patch {i}: {anomaly_count} anomaly pixels detected.\n")
                
                if anomaly_count >= pixel_threshold:
                    # Convert arrays to image format
                    patch_image = Image.fromarray((patch.squeeze() * 255).astype(np.uint8), mode='L')  # Convert to grayscale image
                    prediction_image = Image.fromarray((prediction.squeeze() * 255).astype(np.uint8), mode='L')
                                        
                    # Combine the images side by side
                    combined_width = patch_image.width + prediction_image.width
                    combined_height = max(patch_image.height, prediction_image.height)
                    
                    # Create a new blank image with the combined dimensions
                    combined_image = Image.new('L', (combined_width, combined_height))  # 'L' for grayscale
                    
                    # Paste the patch and prediction images into the combined image
                    combined_image.paste(patch_image, (0, 0))  # Paste patch at (0, 0)
                    combined_image.paste(prediction_image, (patch_image.width, 0))  # Paste prediction to the right of patch
                    
                    # Save the combined image
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
                    combined_filename = os.path.join(output_dir, f"patch_prediction_{i}_{timestamp}.png")
                    combined_image.save(combined_filename)
                    
                    # Accumulate saved image notifications
                    client_messages.append(f"Saved images and prediction for patch {i}: {combined_filename}.\n")
                
                total_anomalies += 1 if anomaly_count > pixel_threshold else 0
            
            # Send a final summary of the prediction results
            summary_message = f"Prediction completed: {total_anomalies} patches with anomalies are more than the detection threshold.\n"
            client_messages.append(summary_message)

            # Join all messages into a single string and send to the client
            final_message = ''.join(client_messages)
            print_with_ts(final_message)
    server.send_message(client, "prediction completed")

# BCE w/ Intersection over Union (IoU)
def iou(y_true, y_pred):
    y_pred = tf.round(y_pred)
    intersection = tf.reduce_sum(y_true * y_pred)
    total = tf.reduce_sum(y_true + y_pred)
    union = total - intersection
    iou = intersection / (union + tf.keras.backend.epsilon())
    return iou

custom_objects = { 'iou': iou }
model = tf.keras.models.load_model(model_path, custom_objects=custom_objects)

PORT=9001 # DONOT CHANGE
server = WebsocketServer(port = PORT)
server.set_fn_new_client(new_client)
server.set_fn_client_left(client_left)
server.set_fn_message_received(message_received)
server.run_forever()




