#include <chrono>
#include <iomanip> // For std::put_time
#include "time_utils.h"

std::string TimeUtils::get_current_time()
{
    // Get the current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    // Get the fractional seconds (milliseconds)
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    // Format the time as a string
    std::stringstream time_stream;
    time_stream << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");

    // Add the milliseconds
    time_stream << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

    return time_stream.str();
}

std::string TimeUtils::get_time_minutes_ago(int minutes)
{
    auto now = std::chrono::system_clock::now();
    auto start_time = now - std::chrono::minutes(minutes);
    auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
    auto start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        start_time.time_since_epoch()) % 1000;

    std::stringstream time_stream;
    time_stream << std::put_time(std::localtime(&start_time_t), "%Y-%m-%d %H:%M:%S")
                << '.' << std::setfill('0') << std::setw(3) << start_time_ms.count();

    return time_stream.str();
}

std::string TimeUtils::get_time_hours_ago(int hours)
{
    auto now = std::chrono::system_clock::now();
    auto start_time = now - std::chrono::hours(hours);
    auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
    auto start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        start_time.time_since_epoch()) % 1000;

    std::stringstream time_stream;
    time_stream << std::put_time(std::localtime(&start_time_t), "%Y-%m-%d %H:%M:%S")
                << '.' << std::setfill('0') << std::setw(3) << start_time_ms.count();

    return time_stream.str();
}

std::chrono::system_clock::time_point TimeUtils::parse_time(const std::string& time_str)
{
    std::istringstream time_stream(time_str);

    // Step 1: Parse the main date and time components (excluding milliseconds)
    std::tm tm = {};
    char dot; // To consume the dot before milliseconds
    int milliseconds;
    time_stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S") >> dot >> milliseconds;

    if (time_stream.fail())
    {
        throw std::runtime_error("Failed to parse time string: " + time_str);
    }

    // Step 2: Convert std::tm to std::time_t
    std::time_t time_t = std::mktime(&tm);

    // Step 3: Convert std::time_t to std::chrono::time_point
    auto time_point = std::chrono::system_clock::from_time_t(time_t);

    // Step 4: Add the milliseconds to the time_point
    time_point += std::chrono::milliseconds(milliseconds);

    return time_point;
}

std::string TimeUtils::get_time_interval(std::chrono::system_clock::time_point current_time_point, std::chrono::system_clock::time_point previous_time_point)
{
    auto duration = current_time_point - previous_time_point;
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // Convert duration to H:M:S.ms format
    int hours = static_cast<int>(duration_ms / (1000 * 60 * 60));
    duration_ms %= (1000 * 60 * 60);
    int minutes = static_cast<int>(duration_ms / (1000 * 60));
    duration_ms %= (1000 * 60);
    int seconds = static_cast<int>(duration_ms / 1000);
    int milliseconds = duration_ms % 1000;

    std::ostringstream duration_stream;
    duration_stream << std::setw(2) << std::setfill('0') << hours << ":"
                    << std::setw(2) << std::setfill('0') << minutes << ":"
                    << std::setw(2) << std::setfill('0') << seconds << "."
                    << std::setw(3) << std::setfill('0') << milliseconds;

    std::string duration_str = duration_stream.str();

    return duration_str;
}

