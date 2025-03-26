#include<iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <future> // Include for std::async
#include "pixbuf_queue.h"

PixbufQueue::PixbufQueue(size_t cap, WorkerFunction workerFunc)
    : capacity(cap), stop_flag(false), worker_func(std::move(workerFunc)) {}

PixbufQueue::~PixbufQueue() {
    stop_worker();
}

void PixbufQueue::enqueue(const std::vector<std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string>>& items) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& item : items) {
            if (queue.size() >= capacity) {
                std::cout << "[" << get_timestamp() << "] queue is full: " << queue.size() << std::endl;
                queue.pop();  // Remove the oldest item if the queue is full
            }
            queue.emplace(item);
            std::cout << "[" << get_timestamp() << "] emplace: " << item.second << std::endl;
        }
    }
    queue_available_cv.notify_one();
}

void PixbufQueue::start_worker() {
    stop_flag = false;
    worker_thread = std::thread(&PixbufQueue::worker_loop, this);
}

void PixbufQueue::stop_worker() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop_flag = true;
    }
    queue_available_cv.notify_all();

    if (worker_thread.joinable()) {
        worker_thread.join();
    }

    // Process remaining items after worker thread exits
    std::vector<std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string>> remaining_tasks;
    {
        std::lock_guard<std::mutex> lock(mutex);
        while (!queue.empty()) {
            remaining_tasks.push_back(queue.front());
            queue.pop();
        }
    }

    if (!remaining_tasks.empty()) {
        std::cout << "[" << get_timestamp() << "] Processing remaining " << remaining_tasks.size() << " items" << std::endl;
        std::vector<std::future<void>> futures;
        for (auto& task : remaining_tasks) {
            std::cout << "[" << get_timestamp() << "] invoke worker_func: " << task.second << std::endl;
            futures.emplace_back(std::async(std::launch::async, worker_func, task.first, task.second));
        }
        for (auto& future : futures) {
            future.get();
        }
    }
}

void PixbufQueue::worker_loop() {
    while (!stop_flag) {
        std::vector<std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string>> tasks;
        {
            std::unique_lock<std::mutex> lock(mutex);
            queue_available_cv.wait(lock, [this] { return !queue.empty() || stop_flag; });
            if (stop_flag && queue.empty()) return;
            while (!queue.empty()) {
                tasks.push_back(queue.front());
                queue.pop();
            }
        }
        
        std::vector<std::future<void>> futures;
        for (auto& task : tasks) {
            std::cout << "[" << get_timestamp() << "] invoke worker_func: " << task.second << std::endl;
            futures.emplace_back(std::async(std::launch::async, worker_func, task.first, task.second));
        }
        for (auto& future : futures) {
            future.get();
        }
    }
    // while (true) {
    //     std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string> task;

    //     {
    //         std::unique_lock<std::mutex> lock(mutex);
    //         queue_available_cv.wait(lock, [this] { return !queue.empty() || stop_flag; });

    //         if (stop_flag && queue.empty()) {
    //             return;
    //         }

    //         task = queue.front();
    //         queue.pop();
    //     }

    //     // Call the user-defined processing function
    //     worker_func(task.first, task.second);
    // }

    // while (!stop_flag) {
    //     std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string> task;
    //     bool has_task = false;

    //     {
    //         std::lock_guard<std::mutex> lock(mutex);
    //         if (!queue.empty()) {
    //             task = queue.front();
    //             queue.pop();
    //             has_task = true;
    //         }
    //     }

    //     if (has_task) {
    //         std::cout << "[" << get_timestamp() << "] invoke worker_func: " + task.second << std::endl;
    //         // worker_func(task.first, task.second);
    //         // Run worker_func asynchronously without blocking the loop
    //         std::async(std::launch::async, worker_func, task.first, task.second);
    //         // try {
    //         //     auto pixbuf = task.first;
    //         //     auto file_path = task.second;
    //         //     std::cout << "[" << get_timestamp() << "] Start saving: " << file_path << std::endl;
    //         //     pixbuf->save(file_path, "png");
    //         //     std::cout << "[" << get_timestamp() << "] End saving: " << file_path << std::endl;
    //         // } catch (const Glib::Error& e) {
    //         //     std::cerr << "Error saving Pixbuf: " << e.what() << std::endl;
    //         // }
    //     } else {
    //         // Sleep briefly to prevent busy-waiting
    //         std::this_thread::sleep_for(std::chrono::milliseconds(5));
    //     }
    // }
}

// #include <boost/lockfree/queue.hpp>
// #include <glibmm/refptr.h>
// #include <gdkmm/pixbuf.h>
// #include <thread>
// #include <atomic>
// #include <iostream>

// class PixbufQueue {
// public:
//     using WorkerFunction = std::function<void(Glib::RefPtr<Gdk::Pixbuf>, std::string)>;

//     PixbufQueue(size_t cap, WorkerFunction workerFunc)
//         : queue(cap), stop_flag(false), worker_func(std::move(workerFunc)) {
//         start_worker();
//     }

//     ~PixbufQueue() {
//         stop_worker();
//     }

//     void enqueue(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf, const std::string& filePath) {
//         if (!queue.push({pixbuf, filePath})) {
//             std::cerr << "Queue is full, dropping frame: " << filePath << std::endl;
//         }
//     }

//     void start_worker() {
//         stop_flag = false;
//         worker_thread = std::thread(&PixbufQueue::worker_loop, this);
//     }

//     void stop_worker() {
//         stop_flag = true;
//         if (worker_thread.joinable()) {
//             worker_thread.join();
//         }
//     }

// private:
//     struct Task {
//         Glib::RefPtr<Gdk::Pixbuf> pixbuf;
//         std::string filePath;
//     };

//     boost::lockfree::queue<Task> queue;  // Lock-free queue
//     std::atomic<bool> stop_flag;         // Atomic stop flag
//     WorkerFunction worker_func;
//     std::thread worker_thread;

//     void worker_loop() {
//         while (!stop_flag) {
//             Task task;
//             if (queue.pop(task)) {
//                 worker_func(task.pixbuf, task.filePath);
//             } else {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Prevent busy-waiting
//             }
//         }
//     }
// };

std::string PixbufQueue::get_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_t);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%H:%M:%S") << '.' 
        << std::setw(3) << std::setfill('0') << now_ms.count();
    return oss.str();
}