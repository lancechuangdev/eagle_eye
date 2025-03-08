#!/usr/bin/env python
# coding: utf-8

import os
import posix_ipc
import mmap
import numpy as np
import torch
import onnxruntime as ort

# from PIL import Image
import cv2
from datetime import datetime
import time
import json
from websocket_server import WebsocketServer
import concurrent.futures
import struct

shared_memory_name_frames = "/ee_shared_memory_frames" # DONOT CHANGE
shared_memory_name_predictions = "/ee_shared_memory_predictions" # DONOT CHANGE
model_path = "/usr/local/share/eagle_eye/model.onnx"
patch_size = 512
rgb_channels = 3
home_dir = os.path.expanduser("~")
output_dir_base = os.path.join(home_dir, "eagle_eye", "detection_projects")
os.makedirs(output_dir_base, exist_ok=True)
executor = concurrent.futures.ThreadPoolExecutor() # ThreadPoolExecutor for saving files
preallocated_pred_shm_size = patch_size * patch_size * rgb_channels * 50 + mmap.PAGESIZE  # Include extra for alignment metadata

def print_with_ts(message):
    print(f"{datetime.now():%Y-%m-%d %H:%M:%S.%f} - {message}")

def get_welcome_message():
    return (f"Welcome to Eagle Eye Anomaly Detection Service!\n"
            f"Configuration:\n"
            f"Patch Size: {patch_size}\n"
            f"Output Directory: {output_dir_base}\n")

def read_frame_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset):
    """
    Reads a single frame from the shared memory based on the frame information.
    """
    # Open the shared memory object with read access
    shm = posix_ipc.SharedMemory(shm_name)

    # Align the offset to the system page size and calculate the size needed for mmap
    aligned_offset = offset - (offset % mmap.PAGESIZE)
    intra_frame_offset = offset % mmap.PAGESIZE
    map_size = frame_size + intra_frame_offset

    # Memory-map the shared memory object
    with mmap.mmap(shm.fd, map_size, mmap.MAP_SHARED, mmap.ACCESS_READ, offset=aligned_offset) as mmap_obj:
        mmap_obj.seek(intra_frame_offset)
        frame_data = mmap_obj.read(frame_size)
    
    # Close the shared memory descriptor
    shm.close_fd()

    # Convert the bytes to a numpy array and reshape
    frame_data = np.frombuffer(frame_data, dtype=np.uint8)
    print(f"frame_data shape: {frame_data.shape}")
    
    frame = frame_data.reshape((frame_height, frame_width, rgb_channels))
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
        if frame_width * frame_height * rgb_channels != frame_size:
            print("Error: frame width x height x channels is not equal to the frame size.")
            continue

        # Read the frame from shared memory
        frame = read_frame_from_shared_memory(shm_name, frame_size, frame_width, frame_height, offset)
        frames.append((frame, serial_number))
    
    return frames

def build_batch_from_frames(frames, frame_width, frame_height):
    patches = []
    
    for frame, _ in frames:
        # Normalize frame to [0, 1] range
        frame = frame.astype(np.float32) / 255.0

        # Calculate number of patches in x and y directions
        num_patches_x = frame_width // patch_size
        num_patches_y = frame_height // patch_size
        print(f"num_patches_x: {num_patches_x}, num_patches_y: {num_patches_y}")

        for j in range(num_patches_y):
            for i in range(num_patches_x):
                patch = frame[j * patch_size:(j + 1) * patch_size,
                              i * patch_size:(i + 1) * patch_size]
                # Convert patch to torch tensor and change channel order from HWC to CHW
                patch_tensor = torch.from_numpy(patch).permute(2, 0, 1)
                patches.append(patch_tensor)

            # Handle the remaining part as a smaller patch, if any
            remainder = frame_width % patch_size
            if remainder > 0:
                start_x = frame_width - patch_size
                last_patch = frame[j * patch_size:(j + 1) * patch_size,
                                   start_x:start_x + patch_size]
                last_patch_tensor = torch.from_numpy(last_patch).permute(2, 0, 1)
                patches.append(last_patch_tensor)
    
    # Stack all patch tensors into one tensor: [N, C, H, W]
    batch_tensor = torch.stack(patches)
    print(f"batch_tensor shape: {batch_tensor.shape}")
    return batch_tensor

# Called for every client connecting (after handshake)
def new_client(client, server):
    print_with_ts("New client connected and was given id %d" % client['id'])
    welcome_message = get_welcome_message()
    print_with_ts(welcome_message)
    
# Called for every client disconnecting
def client_left(client, server):
    print_with_ts("Client(%d) disconnected" % client['id'])

def save_images_and_transaction(output_path, frames_data, frames_metadata, anomaly_data, anomaly_metadata, transaction_json):
    """Save frames, predictions, and transaction JSON in a separate thread."""
    try:
        # Ensure the directory exists
        os.makedirs(output_path, exist_ok=True)

        # Save frames
        for i, metadata in enumerate(frames_metadata):
            file_name = metadata['file_name']
            frame_data = frames_data[i][0] # frame is the first element of frame_data, which is a tuple of (frame, serial_number)
            frame_data = frame_data.astype(np.uint8) # Ensure the frame is in uint8 format
            cv2.imwrite(file_name, frame_data)

        # Save predictions
        for i, metadata in enumerate(anomaly_metadata):
            file_name = metadata['file_name']
            anomaly_map = anomaly_data[i]
            cv2.imwrite(file_name, anomaly_map)

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
    project_name = data.get('project_name', 'ad-hoc') or 'ad-hoc'
    transaction_id = data.get('transaction_id', 0)
    transaction_datetime = data.get('transaction_datetime', '')
    transaction_type = data.get('transaction_type', 'detection')
    confidence_threshold = data.get('confidence_threshold', 0.5)
    frame_width = data.get('frame_width', 0)
    frame_height = data.get('frame_height', 0)
    total_anomalies = 0
    output_path = os.path.join(output_dir_base, project_name, 'detection_results', transaction_id)
    num_frames = 0
    num_patches = 0
    anomaly_metadata = []

    # Build the initial transaction json object
    transaction_json = {
        "transaction_id": transaction_id,
        "transaction_datetime": transaction_datetime,
        "transaction_type": transaction_type,
        "patch_size": patch_size,
        "confidence_threshold": confidence_threshold,
        "frame_width": frame_width,
        "frame_height": frame_height
    }

    if frames_array and transaction_id:
        # Notify client about prediction start
        print_with_ts(f"Transaction {transaction_id} started: Starting prediction on {len(frames_array)} image(s)\n")

        # Read frames from shared memory
        frames_data = read_frames_from_shared_memory(shared_memory_name_frames, frame_width, frame_height, frames_array)
        num_frames = len(frames_data)
        transaction_json["num_frames"] = num_frames

        # Build batch from frames
        input_patches = build_batch_from_frames(frames_data, frame_width, frame_height)
        print(f"input_patches shape: {input_patches.shape}")

        # Prepare input dictionary for ONNX
        input_name = session.get_inputs()[0].name
        onnx_inputs = {input_name: input_patches.numpy()}

        # Run ONNX inference
        print("Running ONNX inference...")
        start_time = time.time()
        onnx_outputs = session.run(None, onnx_inputs)
        end_time = time.time()
        print(f"Inference Time: {(end_time - start_time) * 1000:.2f} ms")

        # Extract model outputs
        anomaly_scores = onnx_outputs[0].flatten()  # Assuming shape (N,1) -> Flatten to (N,)
        pred_labels = onnx_outputs[1].flatten()  # Assuming shape (N,1) -> Flatten to (N,)
        anomaly_maps = onnx_outputs[2]  # Shape (N,1,256,256)
        pred_masks = onnx_outputs[3]  # Shape (N,1,256,256)

        aligned_offsets = []
        total_sh_mem_size = 0
        anomaly_data = []
        num_patches = input_patches.shape[0]

        for i in range(num_patches):
            anomaly_score = anomaly_scores[i].item()  # Convert NumPy scalar to Python scalar
            if anomaly_score >= confidence_threshold:
                anomaly_map = anomaly_maps[i, 0, :, :]
                
                # Resize anomaly map to (patch_size, patch_size)
                anomaly_map_resized = cv2.resize(anomaly_map, (patch_size, patch_size), interpolation=cv2.INTER_CUBIC)
                
                # Normalize the resized anomaly map
                a_min, a_max = anomaly_map_resized.min(), anomaly_map_resized.max()
                anomaly_map_norm = (anomaly_map_resized - a_min) / (a_max - a_min) if a_max - a_min > 1e-8 else anomaly_map_resized

                # Invert the anomaly map so defects appear red
                anomaly_map_norm = 1 - anomaly_map_norm

                # Apply heatmap coloring
                anomaly_map_colored = cv2.applyColorMap((anomaly_map_norm * 255).astype(np.uint8), cv2.COLORMAP_JET)
                anomaly_map_colored = cv2.cvtColor(anomaly_map_colored, cv2.COLOR_BGR2RGB)

                # Calculate aligned offset
                aligned_offset = total_sh_mem_size + (mmap.PAGESIZE - (total_sh_mem_size % mmap.PAGESIZE)) % mmap.PAGESIZE
                aligned_offsets.append(aligned_offset)

                # Write prediction data into shared memory
                memory_pred.seek(aligned_offset)
                memory_pred.write(anomaly_map_colored.tobytes())

                # Update total memory usage
                total_sh_mem_size = aligned_offset + patch_size * patch_size * rgb_channels

                # Build the predictions metadata
                anomaly_metadata.append({
                    "prediction_id": i,
                    "anomaly_score": anomaly_score,
                    "file_name": os.path.join(output_path, f"prediction_{i}.png")
                })
                anomaly_data.append(anomaly_map_colored)
                total_anomalies += 1

        # Pack aligned offsets into metadata and store at the end of the shared memory
        metadata_offset = preallocated_pred_shm_size - len(aligned_offsets) * 4  # Assuming uint32_t offsets
        metadata = struct.pack(f'{len(aligned_offsets)}I', *aligned_offsets)
        memory_pred.seek(metadata_offset)
        memory_pred.write(metadata)

        # Add prediction-related json props
        transaction_json["predictions"] = anomaly_metadata
        transaction_json["total_anomalies"] = total_anomalies

        # Build the frames metadata
        frames_metadata = []
        for i, (frame, serial_number) in enumerate(frames_data):
            frames_metadata.append({
                "frame_id": i,
                "serial_number": serial_number,
                "file_name": os.path.join(output_path, f"frame_{i}.png"),
            })

        # Add frames metadata json prop
        transaction_json["frames"] = frames_metadata

        if total_anomalies > 0:
            # Dispatch saving task to another thread
            executor.submit(save_images_and_transaction, output_path, frames_data, frames_metadata, anomaly_data, anomaly_metadata, transaction_json)

        # Print a final summary of the prediction results
        print_with_ts(f"Transaction {transaction_id} completed: {total_anomalies} patches with anomalies are more than the detection threshold.\n")

    result_json = {
        "transaction_id": transaction_id,
        "status": "complete",
        "total_anomalies": total_anomalies,
        "predictions": [meta["prediction_id"] for meta in anomaly_metadata],
        "patch_size": patch_size    
    }
    result = json.dumps(result_json)
    server.send_message(client, result)

# Create ONNX runtime session
print("Creating ONNX runtime session...")
sess_opts = ort.SessionOptions()
providers = [
    ('TensorrtExecutionProvider', {
        'device_id': 0, # Select GPU to execute
        'trt_max_workspace_size': 4 * 1024 * 1024 * 1024, # Allocate 4GB GPU memory
        'trt_fp16_enable': True, # Enable FP16 precision for faster inference  
    }),
    ('CUDAExecutionProvider', {
        'device_id': 0,
        'arena_extend_strategy': 'kNextPowerOfTwo',
        'gpu_mem_limit': 4 * 1024 * 1024 * 1024,
        'cudnn_conv_algo_search': 'EXHAUSTIVE',
        'do_copy_in_default_stream': True,
    })
]
session = ort.InferenceSession(model_path, sess_options=sess_opts, providers=providers)
print("Session Execution Provider:", session.get_providers())

# Create shared memory for prediction
shm_pred = posix_ipc.SharedMemory(shared_memory_name_predictions, posix_ipc.O_CREAT, size=preallocated_pred_shm_size)
memory_pred = mmap.mmap(shm_pred.fd, shm_pred.size)
shm_pred.close_fd()

PORT=9001 # DONOT CHANGE
server = WebsocketServer(port = PORT)
server.set_fn_new_client(new_client)
server.set_fn_client_left(client_left)
server.set_fn_message_received(message_received)
server.run_forever()