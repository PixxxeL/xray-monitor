#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <ctime>
#include <boost/json.hpp>


namespace json = boost::json;

namespace utils {
    std::string formatTime(time_t time);
    std::string toLower(const std::string& input);
    std::string readFile(const std::string& filepath);
    json::value parseJsonFile(const std::string& filepath);
    std::string executeCommand(const std::string& command);
    void ensurePathExists(const std::string& filePath);
    std::string escapeMDv2(const std::string& text);
}

#endif
