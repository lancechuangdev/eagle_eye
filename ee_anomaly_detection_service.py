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

def read_frames_from_shared_memory(shm_name, frames_array):
    frames = []

    for frame_data in frames_array:
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
        frame = read_frame_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset)

        # Reshape the frame to patch_size height
        reshaped_frame = frame[:patch_size, :]
        print(f"reshaped_frame shape: {reshaped_frame.shape}")

        frames.append((reshaped_frame, frame_width, frame_height))
    
    return frames
    

def build_batch_from_frames(frames, patch_size, shared_memory_name):
    batch = []
    for frame, frame_width, _ in frames:        
        # Normalize frame to [0, 1] range
        frame = frame.astype(np.float32) / 255.0

        # Extract and reshape each patch_size*patch_size patch
        num_patches = frame_width // patch_size
        # print(f"num_patches: {num_patches}")
        for i in range(num_patches):
            patch = frame[:, i * patch_size:(i + 1) * patch_size]
            patch = patch.reshape((patch_size, patch_size, 1))  # Reshape to (patch_size, patch_size, 1)
            batch.append(patch)

        # Handle the remaining part as a smaller patch, if any
        remainder = frame_width % patch_size
        # print(f"remainder: {remainder}")

        if remainder > 0:
            # Adjust the start position for the last patch so it aligns properly
            start_x = frame_width - patch_size
            last_patch = frame[:, start_x:start_x + patch_size]
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
    frames_array = data.get('frames', [])
    transaction_id = data.get('transaction_id', 0)
    total_anomalies = 0
    output_path = os.path.join(output_dir, transaction_id)

    # Build the initial transaction json object
    transaction_json = {
        "transaction_id": transaction_id,
        "patch_size": patch_size
    }

    if frames_array and transaction_id:
        # Notify client about prediction start
        initial_message = f"Transaction {transaction_id} started: Starting prediction on {len(frames_array)} image(s)\n"

        # Read frames from shared memory
        frames = read_frames_from_shared_memory(shared_memory_name, frames_array)
        transaction_json["num_frames"] = len(frames)

        # Build batch from frames
        batch = build_batch_from_frames(frames, patch_size, shared_memory_name)
        transaction_json["num_patches"] = len(batch)

        if batch:
            # Convert list to numpy array with batch shape (num_patches, patch_size, patch_size, 1)
            frame_batch = np.array(batch)
            # print(f"frame_batch shape: {frame_batch.shape}")
            
            # Perform prediction on the entire batch
            predictions = model.predict(frame_batch)
            predictions = (predictions > confidence_threshold).astype(np.float32)
            # print(f"predictions shape: {predictions.shape}")
            
            prediction_files = []

            # List to accumulate messages to send to the client
            client_messages = [initial_message]
            
            # Check each prediction for anomaly
            for i, prediction in enumerate(predictions):
                # Threshold check for anomalies
                anomaly_count = np.sum(prediction == 1)
                
                # Accumulate per-patch anomaly count in client_messages
                client_messages.append(f"Patch {i}: {anomaly_count} anomaly pixels detected.\n")
                
                if anomaly_count >= pixel_threshold:
                    # Convert arrays to image format
                    prediction_image = Image.fromarray((prediction.squeeze() * 255).astype(np.uint8), mode='L')

                    # Build the prediction file name
                    prediction_filename = os.path.join(output_path, f"prediction_{i}.png")
                    prediction_files.append((prediction_image, i, prediction_filename))
                    total_anomalies += 1

            if total_anomalies > 0:
                frames_data = []
                predictions_data = []

                # Ensure the directory exists
                os.makedirs(output_path, exist_ok=True)

                for i, (frame, frame_width, frame_height) in enumerate(frames):
                    # Convert the reshaped frame to an image
                    frame = Image.fromarray(frame.astype(np.uint8))
                    # Save the frame to a file
                    frame_filename = os.path.join(output_path, f"frame_{i}.png")
                    frame.save(frame_filename)
                    client_messages.append(f"Saved frame {i}: {frame_filename}.\n")

                    # Add frame info to list
                    frames_data.append({
                        "frame_id": i,
                        "frame_width": frame_width,
                        "frame_height": frame_height,
                        "filename": frame_filename
                    })

                for prediction_image, i, prediction_filename in prediction_files:
                    prediction_image.save(prediction_filename)
                    client_messages.append(f"Saved prediction for patch {i}: {prediction_filename}.\n")
                    # Add prediction info to list
                    predictions_data.append({
                        "prediction_id": i,
                        "filename": prediction_filename
                    })

                # Update the transaction JSON structure
                transaction_json["frames"] = frames_data
                transaction_json["predictions"] = predictions_data
                transaction_json["total_anomalies"] = total_anomalies

                # Write dictionary to a JSON file with indentation
                with open(os.path.join(output_path, "transaction_data.json"), "w") as trans_json_file:
                    json.dump(transaction_json, trans_json_file, indent=4)

            # Send a final summary of the prediction results
            summary_message = f"Transaction {transaction_id} completed: {total_anomalies} patches with anomalies are more than the detection threshold.\n"
            client_messages.append(summary_message)

            # Join all messages into a single string and send to the client
            final_message = ''.join(client_messages)
            print_with_ts(final_message)

    result_json = {
        "transaction_id": transaction_id,
        "status": "complete",
        "total_anomalies": total_anomalies
    }
    result = json.dumps(result_json)
    server.send_message(client, result)

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




