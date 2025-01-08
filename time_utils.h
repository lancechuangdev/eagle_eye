#ifndef EAGLE_EYE_TIME_UTILS_H
#define EAGLE_EYE_TIME_UTILS_H

#include <string>

class TimeUtils
{
public:
    static std::string get_current_time();
    static std::string get_formatted_time(std::chrono::system_clock::time_point tp);
    static std::string get_time_minutes_ago(int minutes);
    static std::string get_time_hours_ago(int hours);
    static std::chrono::system_clock::time_point parse_time(const std::string& time_str);
    static std::string get_time_interval(std::chrono::system_clock::time_point current_time_point, std::chrono::system_clock::time_point previous_time_point); // H:M:S.ms format
};

#endif // EAGLE_EYE_TIME_UTILS_H