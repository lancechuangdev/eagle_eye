
#ifndef EAGLE_EYE_RETENTION_MANAGER_H
#define EAGLE_EYE_RETENTION_MANAGER_H

#include <filesystem>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

class RetentionManager
{
public:
    void archive();
    void enforce_archive_retention();
    void enforce_daily_limit();
    // void start_daily_limit_enforcer(std::chrono::minutes interval);
    // void stop_daily_limit_enforcer();

private:
    // std::thread m_enforcer_thread;
    // std::atomic<bool> m_stop_enforcer{false};
    // std::condition_variable m_stop_enforcer_cv; // Condition variable
    // std::mutex m_enforcer_mutex; // Mutex for condition variable
    void enforce_daily_limit(const std::filesystem::path& path, int max_per_day);
};

#endif