#ifndef EAGLE_EYE_FRAME_QUEUE_H
#define EAGLE_EYE_FRAME_QUEUE_H

#include <mutex>
#include <vector>
#include "frame_data.h"

class FrameQueue {
public:
    // Constructor to set the capacity
    explicit FrameQueue(size_t capacity);

    // Enqueue data into the queue
    void enqueue(FrameData frameData);

    // Dequeue data from the queue
    bool dequeue(FrameData& outFrame);

    // Clear the queue
    void clear();

    // Check if the queue is empty
    bool isEmpty() const;

    // Check if the queue is full
    bool isFull() const;

    // Get the frame queue size
    size_t get_size() const;

private:
    // Circular buffer to store FrameData
    std::vector<FrameData> buffer;

    // Queue indices and capacity
    size_t head;
    size_t tail;
    size_t capacity;
    size_t size;

    // Mutex for thread safety
    mutable std::mutex mtx;
};

#endif // EAGLE_EYE_FRAME_QUEUE_H