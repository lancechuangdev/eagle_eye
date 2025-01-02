#ifndef EAGLE_EYE_TIME_UTILS_H
#define EAGLE_EYE_TIME_UTILS_H

#include <string>

class TimeUtils
{
public:
    static std::string get_current_time();
    static std::string get_time_minutes_ago(int minutes);
    static std::string get_time_hours_ago(int hours);
    static std::chrono::system_clock::time_point parse_time(const std::string& time_str);
};

#endif // EAGLE_EYE_TIME_UTILS_H