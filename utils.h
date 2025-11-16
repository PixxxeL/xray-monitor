#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <ctime>

namespace utils {
    std::string formatTime(time_t time);
    std::string escapeJsonString(const std::string& input);
}

#endif
