
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
    // void archive();
    // void enforce_archive_retention();
    // void enforce_daily_limit();

private:
    // void enforce_daily_limit(const std::filesystem::path& path, int max_per_day);
};

#endif