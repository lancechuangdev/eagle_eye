#ifndef PIXBUF_QUEUE_H
#define PIXBUF_QUEUE_H

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <gdkmm.h>
#include <glibmm.h>

class PixbufQueue {
public:
    using WorkerFunction = std::function<void(Glib::RefPtr<Gdk::Pixbuf>, const std::string&)>;

    explicit PixbufQueue(size_t capacity, WorkerFunction workerFunc);
    ~PixbufQueue();

    void enqueue(const std::vector<std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string>>& items);
    void start_worker();
    void stop_worker();

private:
    std::queue<std::pair<Glib::RefPtr<Gdk::Pixbuf>, std::string>> queue;
    size_t capacity;
    std::mutex mutex;
    std::condition_variable queue_available_cv;
    bool stop_flag;

    std::thread worker_thread;
    WorkerFunction worker_func;  // Function to process each Pixbuf

    void worker_loop();
    std::string get_timestamp();
};

#endif // PIXBUF_QUEUE_H