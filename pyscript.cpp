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
from websocket_server import WebsocketServer
import json

shm_name = ''
model_path = ''
patch_size = 0
confidence_threshold = 0
pixel_threshold = 0
output_dir = ''
PORT=9004
model=None

def read_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset):    
    # Open the shared memory object
    shm = posix_ipc.SharedMemory(shm_name)

    # Map the shared memory object into Python's address space
    with mmap.mmap(shm.fd, frame_size, mmap.MAP_SHARED, access=mmap.ACCESS_READ, offset=offset) as mmap_obj:
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
        
        # Normalize frame to [0, 1] range
        reshaped_frame = reshaped_frame.astype(np.float32) / 255.0

        # Extract and reshape each patch_size*patch_size patch
        num_patches = frame_width // patch_size
        print(f"num_patches: {num_patches}")
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
            last_patch = np.pad(last_patch, ((0, 0), (0, patch_size - remainder)), mode='constant')
            last_patch = last_patch.reshape((patch_size, patch_size, 1))
            batch.append(last_patch)
    
    return batch

# Called for every client connecting (after handshake)
def new_client(client, server):
    print("New client connected and was given id %d" % client['id'])
    server.send_message_to_all("Hey all, a new client has joined us")
    
# Called for every client disconnecting
def client_left(client, server):
    print("Client(%d) disconnected" % client['id'])

# Called when a client sends a message
def message_received(client, server, message):
    global shm_name, model_path, patch_size, confidence_threshold, pixel_threshold, output_dir, model

    print("Client(%d) said: %s" % (client['id'], message))
    data = json.loads(message)

    new_shm_name = data['shm_name']
    if new_shm_name:
        shm_name = new_shm_name

    new_model_path = data['model_path']
    if new_model_path:
        if new_model_path != model_path:
            # Define the custom objects dictionary
            custom_objects = {
                'iou': iou
            }
            model = tf.keras.models.load_model(model_path, custom_objects=custom_objects)
            print(f"model loaded")
            model_path = new_model_path

    new_patch_size = data['patch_size']
    if new_patch_size:
        patch_size = new_patch_size

    new_confidence_threshold = data['confidence_threshold']
    if new_confidence_threshold:
        confidence_threshold = new_confidence_threshold

    new_pixel_threshold = data['pixel_threshold']
    if new_pixel_threshold:
        pixel_threshold = new_pixel_threshold
    
    new_output_dir = data['output_dir']
    if new_output_dir:
        output_dir = new_output_dir

    frames = data['frames']
    if frames:
        # Build batch from frames
        batch = build_batch_from_frames(frames, patch_size, shm_name)

        if batch:
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
        server.send_message(client, "prediction completed")

def main():
    print("main started")
    server = WebsocketServer(port = PORT)
    server.set_fn_new_client(new_client)
    server.set_fn_client_left(client_left)
    server.set_fn_message_received(message_received)
    server.run_forever()

if __name__ == "__main__":
    main()
)";