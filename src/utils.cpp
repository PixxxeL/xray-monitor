#include "utils.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <memory>
#include <boost/filesystem.hpp>


namespace json = boost::json;
namespace fs = boost::filesystem;

std::string utils::formatTime(time_t time) {
    std::tm* tm = std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string utils::toLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

std::string utils::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    if (content.empty()) {
        throw std::runtime_error("File " + filepath + " is empty");
    }
    return content;
}

json::value utils::parseJsonFile(const std::string& filepath) {
    try {
        return json::parse(utils::readFile(filepath));
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON " + filepath + ": " + std::string(e.what()));
    }
}

std::string utils::executeCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed for command: " + command);
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

void utils::ensurePathExists(const std::string& filePath) {
    fs::path path(filePath);
    fs::path dir = path.parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        fs::create_directories(dir);
    }
}
